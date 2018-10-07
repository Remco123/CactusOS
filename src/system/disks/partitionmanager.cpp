#include <system/disks/partitionmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);

void PartitionManager::DetectAndLoadFilesystems(DiskManager* disks /*, FilesystemManager* filesystems */)
{
    printf("Detecting partitions on disks\n");
    common::uint32_t numDisks = disks->numDisks;

    for(int i = 0; i < numDisks; i++)
    {
        uint8_t* buf = new uint8_t[512];
        char ret = disks->allDisks[i]->ReadSector(0, buf);
        if(ret == 0)
        {
            MasterBootRecord* mbr = (MasterBootRecord*)buf;
            if(mbr->magicnumber != 0xAA55)
            {
                printf("MBR Magic Number is not correct\n");
                printf("Instead it was: "); printfHex16(mbr->magicnumber); printf("\n");
                continue;
            }
            //Loop trough partitions
            for(int p = 0; p < 4; p++)
            {
                if(mbr->primaryPartitions[p].partition_id == 0x00)
                    continue;

                printf("- Disk "); printf(Convert::IntToString(i));
                switch(disks->allDisks[i]->type)
                {
                    case DiskType::CD:
                        printf(" (CDROM) ");
                        break;
                    case DiskType::Floppy:
                        printf(" (Floppy) ");
                        break;
                    case DiskType::HardDisk:
                        printf(" (HDD) ");
                        break;
                    default:
                        printf(" (ERROR) ");
                        break;
                }
                
                printf("Partition: "); printf(Convert::IntToString(p));
                if(mbr->primaryPartitions[p].bootable == 0x80)
                    printf(" bootable=true ID: ");
                else
                    printf(" bootable=false ID: ");
                
                printfHex(mbr->primaryPartitions[p].partition_id);
                printf(" Sectors: "); printf(Convert::IntToString(mbr->primaryPartitions[p].length));

                AssignVFS(mbr->primaryPartitions[p], disks->allDisks[i]);

                printf("\n");
            }
        }
        else
        {
            printf("Error reading disk: "); printf(Convert::IntToString(i)); printf(" Code: "); printfHex(ret); printf("\n");
        }
        delete buf;
    }
}

void PartitionManager::AssignVFS(PartitionTableEntry partition, Disk* disk)
{
    if(partition.partition_id == 0x0B)
    {
        printf(" FAT32");
    }
    else if(partition.partition_id == 0xCD)
    {
        printf(" ISO9660");
    }
}