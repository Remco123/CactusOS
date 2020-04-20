#include <installer/installer.h>
#include <installer/textgui.h>
#include <system/system.h>
#include <core/power.h>
#include <system/vfs/fat32.h>

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
        TextGUI::DrawString(Convert::IntToString(divide64(d->size, 1_MB)), VGA_WIDTH - 8, optionStart + count);
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

    uint32_t sectors = divide64(selectedDisk->size, 512);
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
        coreSize = align_up(coreSize, 512);
    
    // Create memory for core image
    uint8_t* coreBuffer = new uint8_t[coreSize];
    MemoryOperations::memset(coreBuffer, 0, coreSize);

    // And read the file
    if(System::vfs->ReadFile("B:\\setup\\core.img", coreBuffer) != 0)
        SetupError();

    /////////////////////
    // Create FAT32 Partition
    /////////////////////
    TextGUI::StatusBar("Creating FAT32 Partition", 60);
    CreateFatPartition(newMBR.primaryPartitions);  //We use entry 0, makes the most sense.

    ////////////////////
    // Write new MBR to disk
    ////////////////////
    TextGUI::StatusBar("Writing new MBR + Bootloader to disk", 80);
    if(selectedDisk->WriteSector(0, (uint8_t*)&newMBR) != 0)
        SetupError();

    ////////////////////
    // Write core.img to disk
    ////////////////////
    TextGUI::StatusBar("Writing core.img to disk", 85);
    for(uint32_t sector = 0; sector < (coreSize/512); sector++)
        if(selectedDisk->WriteSector(sector + 1, coreBuffer + sector*512) != 0)
            SetupError();
    
    TextGUI::StatusBar("Cleaning up....", 90);

    //Free buffer
    delete coreBuffer;
}

void Installer::SetupError()
{
    //TextGUI::ClearScreen(VGA_COLOR_BLUE);
    TextGUI::DrawString("Error while installing", 0, 0);
}

uint32_t alignSector(uint32_t sectors, uint32_t clusterSize)
{
	return (sectors + clusterSize - 1) & ~(clusterSize - 1);
}

inline uint32_t cdiv(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

void setClusterValue(uint8_t* fat, int cluster, uint32_t value)
{
    value &= 0xFFFFFFF;
	fat[4 * cluster] = (uint8_t)(value & 0x000000FF);
	fat[(4 * cluster) + 1] = (uint8_t)((value & 0x0000FF00) >> 8);
	fat[(4 * cluster) + 2] = (uint8_t)((value & 0x00FF0000) >> 16);
	fat[(4 * cluster) + 3] = (uint8_t)((value & 0xFF000000) >> 24);
}

void Installer::CreateFatPartition(PartitionTableEntry* pEntry)
{
    // First fill in the new values for partition entry
    pEntry->bootable = (1<<7);
    pEntry->start_lba = 2048;
    pEntry->length = (selectedDisk->size / 512) - pEntry->start_lba - (1_MB / 512);
    pEntry->partition_id = 0x0B;
    // Legacy CHS Values
    { // Begin
        int head,track,sector;
        LBAToCHS(pEntry->start_lba, &head, &track, &sector);
        pEntry->start_head = head;
        pEntry->start_cylinder = track;
        pEntry->start_sector = sector;
    }
    { // End
        int head,track,sector;
        LBAToCHS(pEntry->start_lba + pEntry->length, &head, &track, &sector);
        pEntry->end_head = head;
        pEntry->end_cylinder = track;
        pEntry->end_sector = sector;
    }


    // Create BIOS Parameter Block structure in memory
    FAT32_BPB biosParameterBlock;
    MemoryOperations::memset(&biosParameterBlock, 0, sizeof(FAT32_BPB));

    uint32_t sizeMB = ((pEntry->length / 1024) * 512) / 1024;

    ///////////////////////
    // BIOS Parameter Block values
    ///////////////////////
    biosParameterBlock.bootCode[0] = 0xEB;
    biosParameterBlock.bootCode[1] = 0x3C;
    biosParameterBlock.bootCode[2] = 0x90;

    MemoryOperations::memcpy(biosParameterBlock.Oem_Id, "CactusOS", 8);
    biosParameterBlock.bytesPerSector = 512;
    biosParameterBlock.SectorsPerCluster = sizeMB > 32_KB ? 64 : sizeMB > 16_KB ? 32 : sizeMB > 8_KB ? 16 : sizeMB > 260 ? 8 : 1; //_KB Can be a bit misleading, we are comparing MB with MB
    biosParameterBlock.ReservedSectors = 32;
    biosParameterBlock.NumOfFats = 2;
    biosParameterBlock.NumDirEntries = 0;
    biosParameterBlock.TotalSectorsSmall = 0;
    biosParameterBlock.MediaDescriptorType = 0xF8;
    biosParameterBlock.SectorsPerFat12_16 = 0;
    biosParameterBlock.SectorsPerTrack = 32;
    biosParameterBlock.NumHeads = 64;
    biosParameterBlock.HiddenSectors = pEntry->start_lba;
    biosParameterBlock.TotalSectorsBig = pEntry->length;

    // FAT32 Specific values
    // Calculate Sectors Per Fat
    uint32_t nClusters = divide64(((long long)biosParameterBlock.TotalSectorsBig * 512 + biosParameterBlock.NumOfFats * 8), ((int)biosParameterBlock.SectorsPerCluster * 512 + biosParameterBlock.NumOfFats * 4));
    biosParameterBlock.SectorsPerFat32 = cdiv((nClusters + 2) * 4, 512);
    biosParameterBlock.SectorsPerFat32 = alignSector(biosParameterBlock.SectorsPerFat32, biosParameterBlock.SectorsPerCluster);
    
    biosParameterBlock.Flags = 0; // Active = 0 and No Mirroring
    biosParameterBlock.FATVersionNum = 0; // 0.0
    biosParameterBlock.RootDirCluster = 2;
    biosParameterBlock.FSInfoSector = 1;
    biosParameterBlock.BackupBootSector = 0; // We do not provide a backup of the boot sector
    biosParameterBlock.DriveNum = 0x80;
    biosParameterBlock.WinNTFlags = 0;
    biosParameterBlock.Signature = 0x29;

    char volumeID[4] = {System::rtc->GetSecond(), System::rtc->GetDay(), System::rtc->GetMonth(), System::rtc->GetYear() - 2000};
    biosParameterBlock.VolumeIDSerial = *(uint32_t*)volumeID;
    MemoryOperations::memcpy(biosParameterBlock.VolumeLabel, "CactusOS HD", 11);
    MemoryOperations::memcpy(biosParameterBlock.SystemIDString, "FAT32   ", 8);
    biosParameterBlock.BootSignature = 0xAA55;

    // Create FSInfo Structure
    FAT32_FSInfo fsInfo;
    MemoryOperations::memset(&fsInfo, 0, sizeof(FAT32_FSInfo));

    ///////////////////////
    // FSInfo values
    ///////////////////////
    fsInfo.signature1 = 0x41615252;
    fsInfo.signature2 = 0x61417272;
    fsInfo.lastFreeCluster = 0xFFFFFFFF; //Unknown
    fsInfo.startSearchCluster = 0xFFFFFFFF; //Unknown, perhaps change those values in the future
    fsInfo.signature3 = 0xAA550000;



    ///////////////////////
    // Create FAT Tables
    ///////////////////////
    uint8_t fatTable[512];
    MemoryOperations::memset(fatTable, 0, 512);

    // Set first to entries
    setClusterValue(fatTable, 0, 0xFFFFFFFF);
    setClusterValue(fatTable, 1, 0xFFFFFFFF);
    fatTable[0] = (uint8_t)biosParameterBlock.MediaDescriptorType; // Not sure why this is needed
    setClusterValue(fatTable, 2, FAT_EOF);

    uint32_t rootDirSize = biosParameterBlock.SectorsPerCluster * biosParameterBlock.bytesPerSector;
    DirectoryEntry rootDir;
    MemoryOperations::memset(&rootDir, 0, sizeof(DirectoryEntry));

    // Fill in root dir values
    rootDir.Attributes = FAT_VOLUME_ID;

    ///////////////////////
    // Write changes to disk
    // Clear reserved sectors first
    uint8_t zeroBuffer[512];
    MemoryOperations::memset(zeroBuffer, 0, 512);
    TextGUI::StatusBar("Cleaning Reserved Sectors", 65);
    for(int i = 0; i < biosParameterBlock.ReservedSectors; i++)
        if(selectedDisk->WriteSector(pEntry->start_lba + i, zeroBuffer) != 0)
            SetupError();

    // Copy FAT's
    TextGUI::StatusBar("Writing FAT Tables", 70);
    for(int i = 0; i < biosParameterBlock.NumOfFats; i++) {
        if(selectedDisk->WriteSector(pEntry->start_lba + biosParameterBlock.ReservedSectors + (i*biosParameterBlock.SectorsPerFat32), fatTable) != 0)
            SetupError();

        // Set rest of FAT to 0
        for(uint32_t c = 0; c < (biosParameterBlock.SectorsPerFat32 - 1); c++)
            if(selectedDisk->WriteSector(pEntry->start_lba + biosParameterBlock.ReservedSectors + (i*biosParameterBlock.SectorsPerFat32) + c + 1, zeroBuffer) != 0)
                SetupError();
    }

    // Copy root directory
    // Use zerobuffer since it is not needed anymore
    TextGUI::StatusBar("Writing root directory", 75);
    MemoryOperations::memcpy(zeroBuffer, &rootDir, sizeof(DirectoryEntry));
    if(selectedDisk->WriteSector(pEntry->start_lba + biosParameterBlock.ReservedSectors + (biosParameterBlock.NumOfFats * biosParameterBlock.SectorsPerFat32), zeroBuffer) != 0)
        SetupError();

    ///////////////////////
    // Write Bios Parameter Block and FSInfo structure to disk
    ///////////////////////
    TextGUI::StatusBar("Writing BPB and FSInfo", 80);
    if(selectedDisk->WriteSector(pEntry->start_lba, (uint8_t*)&biosParameterBlock) != 0)
        SetupError();
    if(selectedDisk->WriteSector(pEntry->start_lba + biosParameterBlock.FSInfoSector, (uint8_t*)&fsInfo) != 0)
        SetupError();
}