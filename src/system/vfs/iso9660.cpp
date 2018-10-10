#include <system/vfs/iso9660.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printbits(uint8_t);

void printlen(char* str, int len)
{
    char* buf = new char[len];
    MemoryOperations::memcpy(buf, str, len);
    buf[len] = '\0';
    printf(buf);
    delete buf;
}

ISO9660::ISO9660(Disk* disk, common::uint32_t start, common::uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->ReadOnly = true;
}

bool ISO9660::Initialize()
{
    printf("Starting ISO9660 fileystem\n");
    printf("Finding Volume Descriptors\n");
    bool FoundPVD = false;
    int Offset = ISO_START_SECTOR;

    uint8_t* readBuffer = new uint8_t[CDROM_SECTOR_SIZE];

    while (FoundPVD == false)
    {
        this->disk->ReadSector(Offset, readBuffer);
        VolumeDescriptor* descriptor = (VolumeDescriptor*)readBuffer;
        printf("Found, type = "); printf(Convert::IntToString(descriptor->Type)); printf("\n");

        if(descriptor->Type == VolumeDescriptorType::PVDescriptor)
        {
            FoundPVD = true;
            printf("Found Primary Volume Descriptor at offset: "); printf(Convert::IntToString(Offset)); printf("\n");

            PrimaryVolumeDescriptor* pvd = (PrimaryVolumeDescriptor*)readBuffer;
            printf("Identifier: "); printf(pvd->id); printf("\n");
            printf("Volume Identifier: "); printf(pvd->volume_id); printf("\n");
            printf("Version: "); printf(Convert::IntToString((uint8_t)pvd->version)); printf("\n");
            printf("Root directory sector: "); printf(Convert::IntToString(pvd->root_directory_record.extent_location)); printf("\n");
            printf("Creation date: "); printlen(pvd->creation_date.Year, 5); printf("-"); printlen(pvd->creation_date.Month + 1, 2); printf("-"); printlen(pvd->creation_date.Day + 1, 2); printf(" : "); printlen(pvd->creation_date.Hour + 1, 2); printf(":"); printlen(pvd->creation_date.Minute + 1, 2); printf(":"); printlen(pvd->creation_date.Second + 1, 2); printf("\n");

            this->primaryVolumeDescriptor = (PrimaryVolumeDescriptor*) MemoryManager::activeMemoryManager->malloc(sizeof(PrimaryVolumeDescriptor));
            MemoryOperations::memcpy(this->primaryVolumeDescriptor, pvd, sizeof(PrimaryVolumeDescriptor));
        }
        else if (descriptor->Type == VolumeDescriptorType::VolumeDescriptorSetTerminator)
        {
            printf("Found end of descriptors\n");
            Offset = this->SizeInSectors + (uint32_t)ISO_START_SECTOR + 1234; //We reached the end of the descriptors
        }                                                                     //                               |
                                                                              //                               |
        if(Offset < (this->SizeInSectors - (uint32_t)ISO_START_SECTOR))       //                               |
            Offset++; //Read the next sector                                  //                               |
        else // <----------------------------------------------------------------------------------------------- So this gets called 
        {
            printf("Could not find Primary Volume Descriptor\n");
            delete readBuffer;
            return false;
        }
    }

    //We found the root directory sector, now lets read it
    this->disk->ReadSector(this->primaryVolumeDescriptor->root_directory_record.extent_location, readBuffer);
    this->rootDirectory = (DirectoryRecord*)MemoryManager::activeMemoryManager->malloc(sizeof(DirectoryRecord));
    MemoryOperations::memcpy(this->rootDirectory, readBuffer, sizeof(DirectoryRecord));

    printf("Root dir length: "); printf(Convert::IntToString(rootDirectory->length)); printf("\n");
    printf("Flags: 0b"); printbits(rootDirectory->flags);

    delete readBuffer;
}