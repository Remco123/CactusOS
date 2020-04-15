#include <installer/installer.h>
#include <installer/textgui.h>
#include <system/system.h>
#include <core/power.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

#define SECTORS_PER_TRACK 80

char* AsciiLogo[] = {
"    ######     ###     ######  ######## ##     ##  ######   #######   ######   ",
"   ##    ##   ## ##   ##    ##    ##    ##     ## ##    ## ##     ## ##    ##  ",
"   ##        ##   ##  ##          ##    ##     ## ##       ##     ## ##        ",
"   ##       ##     ## ##          ##    ##     ##  ######  ##     ##  ######   ",
"   ##       ######### ##          ##    ##     ##       ## ##     ##       ##  ",
"   ##    ## ##     ## ##    ##    ##    ##     ## ##    ## ##     ## ##    ##  ",
"    ######  ##     ##  ######     ##     #######   ######   #######   ######   "
};

char* FileList[] = {
    "B:\\setup\\boot.img",
    "B:\\setup\\welcome.txt",
    "B:\\setup\\warning.txt",
};

Disk* selectedDisk = 0;
uint32_t startLBA = 0;
uint32_t endLBA = 0;
int partitionIndex = 0;

void LBAToCHS(int lba, int *head, int *track, int *sector)
{
	(*head) = (lba % (SECTORS_PER_TRACK * 2)) / SECTORS_PER_TRACK;
	(*track) = (lba / (SECTORS_PER_TRACK * 2));
	(*sector) = (lba % SECTORS_PER_TRACK + 1);
}

void Installer::Run()
{
    TextGUI::DisableCursor();
    TextGUI::ClearScreen(VGA_COLOR_BLUE);

    // Draw Ascii logo
    for(int i = 0; i < sizeof(AsciiLogo) / sizeof(char*); i++)
        TextGUI::DrawString(AsciiLogo[i], 0, i + 1, VGA_COLOR_GREEN);

    TextGUI::StatusBar("Initializing Setup...", 0);
    TextGUI::StatusBar("Checking Setup Files...", 5);
    for(int i = 0; i< sizeof(FileList) / sizeof(char*); i++) {
        TextGUI::StatusBar(FileList[i], 5);
        if(System::vfs->FileExists(FileList[i]) == false)
            SetupError();
    }
    // Display welcome at start of setup
    ShowWelcomeMessage();

    // Next show warning message
    ShowWarningMessage();

    // Show disk selection menu
    ShowDiskSelection();

    // Ask user for disk wipe
    ShowDiskEraseMenu();

    // Configure Master Boot Record
    ShowMBRMenu();

    // Write Master Boot Record
    ShowMBRWriteMenu();

    TextGUI::ClearScreen();
    TextGUI::DrawString("Setup is complete, press enter to reboot", 0, 0);

    TextGUI::StatusBar("Waiting for enter key....", 60);
    while(GetKey() != KEY_ENTER)
        TextGUI::StatusBar("Waiting for enter key.... [Wrong Key]", 60);
    
    Log(Info, "Rebooting System");
    Power::Reboot();
}

char Installer::GetKey()
{
    while(System::keyboardStream->Availible() == 0);

    return System::keyboardStream->Read();
}

void Installer::ShowWelcomeMessage()
{
    int fileSize = System::vfs->GetFileSize("B:\\setup\\welcome.txt");
    char* buf = new char[fileSize + 1];
    buf[fileSize] = '\0';
    if(System::vfs->ReadFile("B:\\setup\\welcome.txt", (uint8_t*)buf) == -1)
        SetupError();
    
    TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString(buf, 0, 0);

    delete buf;
    TextGUI::StatusBar("Waiting for Keypress...", 10);
    while(GetKey() != KEY_ENTER) //Enter Key
        TextGUI::StatusBar("Waiting for Keypress... [Incorrect]", 10);
}

void Installer::ShowWarningMessage()
{
    int fileSize = System::vfs->GetFileSize("B:\\setup\\warning.txt");
    char* buf = new char[fileSize + 1];
    buf[fileSize] = '\0';
    if(System::vfs->ReadFile("B:\\setup\\warning.txt", (uint8_t*)buf) == -1)
        SetupError();
    
    TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString(buf, 0, 0);

    delete buf;
    TextGUI::StatusBar("Waiting for Keypress...", 10);
    while(GetKey() != KEY_ENTER) //Enter Key
        TextGUI::StatusBar("Waiting for Keypress... [Incorrect]", 10);
}

void Installer::ShowDiskSelection()
{
    TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString("Select a disk to install CactusOS to:", 0, 0);
    TextGUI::StatusBar("Select disk.    [Up/Down/Enter]", 10);

    const int optionStart = 5; //Rows between options to start
    int count = 0;
    for(Disk* d : System::diskManager->allDisks) {
        if(d->type != HardDisk)
            continue;
        
        char* nameString = d->identifier ? d->identifier : (char*)"Unnamed Hard Disk";
        TextGUI::DrawString(nameString, 3, optionStart + count);
        TextGUI::DrawString(Convert::IntToString(d->size / 1_MB), VGA_WIDTH - 8, optionStart + count);
        TextGUI::DrawString("MB", VGA_WIDTH - 2, optionStart + count);

        count++;
    }
    if(count == 0)
        SetupError();

    int selectIndex = 0;
    while(selectedDisk == 0) {
        TextGUI::DrawString("-->", 0, optionStart + selectIndex);
        switch(GetKey()) {
            case KEY_ARROW_UP: //Arrow Up
                if(selectIndex > 0) {
                    TextGUI::DrawString("   ", 0, optionStart + selectIndex);
                    selectIndex = 0;
                }
                break;
            case KEY_ARROW_DOWN: //Arrow Down
                if(selectIndex < (count - 1)) {
                    TextGUI::DrawString("   ", 0, optionStart + selectIndex);
                    selectIndex++;
                }
                break;
            case KEY_ENTER: //Enter
                selectedDisk = System::diskManager->allDisks[selectIndex];
                break;
            default:
                continue;
        }
    }
}

void Installer::ShowDiskEraseMenu()
{
    TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString(selectedDisk->identifier ? selectedDisk->identifier : (char*)"Unnamed Hard Disk", 0, 0);
    TextGUI::DrawString("Would you like to completely wipe the selected disk?", 0, 3);
    TextGUI::DrawString("This can be usefull when you want to completely overwrite the system", 0, 4);
    TextGUI::DrawString("Press [Y] for wipe and [N] to continue the setup", 0, 5);
    TextGUI::StatusBar("Waiting for key", 20);

    if(GetKey() != 0x15) //Key is not Y key
        return;

    const uint32_t sectors = selectedDisk->size / 512;
    uint8_t buf[512];
    MemoryOperations::memset(buf, 0, 512);

    TextGUI::StatusBar("Cleaning Disk....", 30);
    for(uint32_t s = 0; s < sectors; s++) {
        selectedDisk->WriteSector(s, buf);
        if(s % 10 == 0) {
            TextGUI::DrawString(Convert::IntToString32(sectors), 0, VGA_HEIGHT - 3);
            TextGUI::DrawString("/", 10, VGA_HEIGHT - 3);
            TextGUI::DrawString(Convert::IntToString32(s), 13, VGA_HEIGHT - 3);
            TextGUI::SetPixel(((double)s/(double)sectors) * (double)VGA_WIDTH, VGA_HEIGHT - 2, TEXT_COLOR, '#', VGA_COLOR_GREEN);
        }
    }
}

void Installer::ShowMBRMenu()
{
    TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString("In this menu you can configure the Master Boot Record of the hard drive", 0, 0);
    TextGUI::DrawString("This contains the partitions of the disk and the initial boot code", 0, 1);
    TextGUI::DrawString("WARNING! During this process the boot loader will be overwritten.", 0, 3);

    TextGUI::StatusBar("Reading current MBR", 40);
    MasterBootRecord currentMBR;
    selectedDisk->ReadSector(0, (uint8_t*)&currentMBR);
    
    TextGUI::DrawString("Select partion entry to use: ", 0, 5);
    for(int i = 0; i < 4; i++) {
        TextGUI::DrawString(Convert::IntToString(i), 0, 6 + i);
        TextGUI::DrawString(": Type: ", 1, 6 + i);
        TextGUI::DrawString(Convert::IntToHexString(currentMBR.primaryPartitions[i].partition_id), 8, 6 + i);
        TextGUI::DrawString("Start: ", 12, 6 + i);
        TextGUI::DrawString(Convert::IntToString32(currentMBR.primaryPartitions[i].start_lba), 19, 6 + i);
        TextGUI::DrawString("Sectors: ", 30, 6 + i);
        TextGUI::DrawString(Convert::IntToString32(currentMBR.primaryPartitions[i].length), 39, 6 + i);
    }
    TextGUI::StatusBar("Press index of partition [0,1,2,3]", 45);
    char key = GetKey();
    while(key != 0x0B && key != 0x02 && key != 0x03 && key != 0x04)
        key = GetKey();
    
    int index = (key == 0x0B) ? 0 : (key - 1);
    startLBA = currentMBR.primaryPartitions[index].start_lba;
    endLBA = startLBA + currentMBR.primaryPartitions[index].length;
    uint32_t scale = 20;

    // Partition start and length need to be given by user
    if(endLBA == 0) {
        TextGUI::DrawString("Enter partition start en end address of LBA:", 0, 10);
        TextGUI::DrawString("Press A to use all of the hard drive", 0, 12);

        ///////////////
        // START LBA
        ///////////////
        uint32_t tmpStartLBA = 0;
        while(startLBA == 0 && endLBA == 0) {
            TextGUI::StatusBar("Enter start LBA address... [-/+ for value, </> for scale]", 50);

            TextGUI::DrawString("                                                                               ", 0, 11);
            TextGUI::DrawString("Start: ", 0, 11);
            TextGUI::DrawString(Convert::IntToString(tmpStartLBA), 7, 11);

            TextGUI::DrawString("Scale: ", 30, 11);
            TextGUI::DrawString(Convert::IntToString32(1 << scale), 37, 11);
            TextGUI::DrawString(Convert::IntToString32((1<<scale)/1_MB), VGA_WIDTH - 10, 11);
            TextGUI::DrawString(" MB", VGA_WIDTH - 3, 11);

            char key2 = GetKey();
            switch(key2) {
                case KEY_LEFT_ARROW:
                    if(scale > 1)
                        scale--;
                    break;
                case KEY_RIGHT_ARROW:
                    if(scale < 30)
                        scale++;
                    break;
                case KEY_MINUS:
                    if((int)(tmpStartLBA - (1<<scale)) > 0)
                        tmpStartLBA -= 1<<scale;
                    break;
                case KEY_PLUS:
                    tmpStartLBA += 1<<scale;
                    break;
                case KEY_ENTER:
                    startLBA = tmpStartLBA;
                    break;
                case KEY_A:
                    startLBA = 0;
                    endLBA = selectedDisk->size/512;
                    break;
                default:
                    break;
            }
        }


        ///////////////
        // END LBA
        ///////////////
        uint32_t tmpEndLBA = startLBA + (10_GB/512);
        while(endLBA == 0) {
            TextGUI::StatusBar("Enter End LBA address... [-/+ for value, </> for scale]", 55);

            TextGUI::DrawString("                                                                               ", 0, 11);
            TextGUI::DrawString("End: ", 0, 11);
            TextGUI::DrawString(Convert::IntToString(tmpEndLBA), 7, 11);

            TextGUI::DrawString("Scale: ", 30, 11);
            TextGUI::DrawString(Convert::IntToString32(1 << scale), 37, 11);
            TextGUI::DrawString(Convert::IntToString32((1<<scale)/1_MB), VGA_WIDTH - 10, 11);
            TextGUI::DrawString(" MB", VGA_WIDTH - 3, 11);

            char key2 = GetKey();
            switch(key2) {
                case KEY_LEFT_ARROW:
                    if(scale > 1)
                        scale--;
                    break;
                case KEY_RIGHT_ARROW:
                    if(scale < 30)
                        scale++;
                    break;
                case KEY_MINUS:
                    if((int)(tmpEndLBA - (1<<scale)) > 0)
                        tmpEndLBA -= 1<<scale;
                    break;
                case KEY_PLUS:
                    tmpEndLBA += 1<<scale;
                    break;
                case KEY_ENTER:
                    endLBA = tmpEndLBA;
                    break;
                case KEY_A:
                    startLBA = 0;
                    endLBA = selectedDisk->size/512;
                    break;
                default:
                    break;
            }
        }
    }
    partitionIndex = index;
}

void Installer::ShowMBRWriteMenu()
{
    TextGUI::ClearScreen();
    Log(Info, "You are about to write the following to the disk and MBR:");
    Log(Info, "    - Partition (%d) from (%x) to (%x)", partitionIndex, startLBA, endLBA);
    Log(Info, "    - Grub stage1 bootcode");
    Log(Info, "    - Make the selected partition bootable");
    Log(Info, "To proceed press the enter key");

    TextGUI::StatusBar("Waiting for enter key....", 70);
    while(GetKey() != KEY_ENTER)
        TextGUI::StatusBar("Waiting for enter key.... [Wrong Key]", 70);
    
    // Read current MBR again
    MasterBootRecord currentMBR;
    selectedDisk->ReadSector(0, (uint8_t*)&currentMBR);

    uint8_t zeroBuf[512]; //A buffer containing all zeros
    MemoryOperations::memset(zeroBuf, 0, 512);

    if(MemoryOperations::memcmp(&currentMBR, zeroBuf, 512) == 0) { //Current MBR is completely zero
        //Fill in some values for the new MBR
        char sign[4] = {'C', 'A', System::rtc->GetMonth(), System::rtc->GetYear() - 2000};
        currentMBR.signature = *(uint32_t*)sign;
        currentMBR.unused = 0;
        currentMBR.magicnumber = 0xAA55;
    }

    // First we fill in the values of the new MBR
    currentMBR.primaryPartitions[partitionIndex].bootable = (1<<7);
    currentMBR.primaryPartitions[partitionIndex].start_lba = startLBA;
    currentMBR.primaryPartitions[partitionIndex].length = endLBA - startLBA;

    // Legacy CHS Values
    { // Begin
        int head,track,sector;
        LBAToCHS(startLBA, &head, &track, &sector);
        currentMBR.primaryPartitions[partitionIndex].start_head = head;
        currentMBR.primaryPartitions[partitionIndex].start_cylinder = track;
        currentMBR.primaryPartitions[partitionIndex].start_sector = sector;
    }
    { // End
        int head,track,sector;
        LBAToCHS(endLBA, &head, &track, &sector);
        currentMBR.primaryPartitions[partitionIndex].end_head = head;
        currentMBR.primaryPartitions[partitionIndex].end_cylinder = track;
        currentMBR.primaryPartitions[partitionIndex].end_sector = sector;
    }

    // Read boot code from CDROM
    uint8_t bootCode[440];
    if(System::vfs->ReadFile("B:\\setup\\boot.img", bootCode) != 0)
        SetupError();

    // Then we copy the boot code used by grub
    MemoryOperations::memcpy(currentMBR.bootloader, bootCode, 440);

    // Finally write it back to the disk
    TextGUI::StatusBar("Updating MBR", 75);
    selectedDisk->WriteSector(0, (uint8_t*)&currentMBR);
}

void Installer::SetupError()
{
    //TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString("Error while installing", 0, 0);
}