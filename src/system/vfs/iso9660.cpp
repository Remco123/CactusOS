#include <system/vfs/iso9660.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

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
            printf("Identifier: "); printf((char*)pvd->Identifier); printf("\n");
            printf("Volume Identifier: "); printf((char*)pvd->VolumeIdentifier); printf("\n");
            printf("Version: "); printfHex(pvd->version); printf("\n");

            printf("root dir size: "); printf(Convert::IntToString(pvd->root_directory.data_length)); printf("\n");
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
            return false;
        }
    }

    delete readBuffer;
}