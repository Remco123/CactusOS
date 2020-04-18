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

    // Start the installation
    ShowInstallScreen();

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

// Perform the installation
void Installer::ShowInstallScreen()
{
    TextGUI::ClearScreen();
    TextGUI::DrawString("You are about to start the installation of CactusOS to your hard drive", 0, 0);
    TextGUI::DrawString("The installation process will contain the following:", 0, 1);
    TextGUI::DrawString("   - Replace the MBR with grub stage1 boot code + partition table", 0, 2);
    TextGUI::DrawString("   - Copy rest of the bootloader to harddisk", 0, 3);
    TextGUI::DrawString("   - Create FAT32 partition as main filesystem", 0, 4);
    TextGUI::DrawString("   - Copy system files to new partition", 0, 5);

    TextGUI::DrawString("Press enter to start the installation", 0, 7);
    TextGUI::StatusBar("Waiting for enter key....", 40);
    while(GetKey() != KEY_ENTER)
        TextGUI::StatusBar("Waiting for enter key.... [Wrong Key]", 40);
    
    /////////////////////////
    // Start of installation
    /////////////////////////

    /////////////////////////
    // Create new MBR with bootloader
    /////////////////////////
    TextGUI::StatusBar("Creating new MBR in memory....", 45);
    MasterBootRecord newMBR;
    MemoryOperations::memset(&newMBR, 0x0, sizeof(MasterBootRecord));

    // Fill in some values
    char sign[4] = {'C', 'A', System::rtc->GetMonth(), System::rtc->GetYear() - 2000};
    newMBR.signature = *(uint32_t*)sign;
    newMBR.unused = 0;
    newMBR.magicnumber = 0xAA55;
    
    // Read bootloader stage1 from cdrom
    uint8_t bootloader[440];
    TextGUI::StatusBar("Reading bootloader stage1 from CD", 50);
    if(System::vfs->ReadFile("B:\\setup\\boot.img", bootloader) != 0)
        SetupError();
    
    // Copy it to our MBR
    MemoryOperations::memcpy(newMBR.bootloader, bootloader, 440);

    //////////////////////////
    // Read core.img from cdrom
    //////////////////////////
    TextGUI::StatusBar("Reading bootloader from CD to disk", 55);
    uint32_t coreSize = System::vfs->GetFileSize("B:\\setup\\core.img");
    if((int)coreSize == -1)
        SetupError();
    
    // Check if size is alligned
    if(coreSize % 512 != 0)
        SetupError();
    
    // Create memory for core image
    uint8_t* coreBuffer = new uint8_t[coreSize];

    // And read the file
    if(System::vfs->ReadFile("B:\\setup\\core.img", coreBuffer) != 0)
        SetupError();

    //TODO: Create partitions
    TextGUI::StatusBar("Creating FAT32 Partition", 60);
    


    



    ////////////////////
    // Write new MBR to disk
    ////////////////////
    TextGUI::StatusBar("Writing new MBR + Bootloader to disk", 65);
    if(selectedDisk->WriteSector(0, (uint8_t*)&newMBR) != 0)
        SetupError();

    ////////////////////
    // Write core.img to disk
    ////////////////////
    for(uint32_t sector = 0; sector < (coreSize/512); sector++)
        if(selectedDisk->WriteSector(sector + 1, coreBuffer + sector*512) != 0)
            SetupError();
    
    TextGUI::StatusBar("Cleaning up....", 70);

    //Free buffer
    delete coreBuffer;
}

void Installer::SetupError()
{
    //TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString("Error while installing", 0, 0);
}