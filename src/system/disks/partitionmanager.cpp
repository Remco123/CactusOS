#include <system/disks/partitionmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printfHex16(uint16_t);

uint8_t Readbuf[512];

void PartitionManager::DetectAndLoadFilesystems(DiskManager* disks , VFSManager* vfs)
{
    printf("Detecting partitions on disks\n");
    common::uint32_t numDisks = disks->numDisks;

    for(int i = 0; i < numDisks; i++)
    {
        char ret = disks->allDisks[i]->ReadSector(0, Readbuf);
        if(ret == 0)
        {
            MasterBootRecord* mbr = (MasterBootRecord*)Readbuf;
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

                AssignVFS(mbr->primaryPartitions[p], disks->allDisks[i], vfs);

                printf("\n");
            }
        }
        else
        {
            printf("Error reading disk: "); printf(Convert::IntToString(i)); printf(" Code: "); printfHex(ret); printf("\n");
        }
    }
}

void PartitionManager::AssignVFS(PartitionTableEntry partition, Disk* disk, VFSManager* vfs)
{
    if(partition.partition_id == 0x0B)
    {
        printf(" FAT32");
    }
    else if(partition.partition_id == 0xCD)
    {
        printf(" ISO9660\n");
        ISO9660* isoVFS = new ISO9660(disk, partition.start_lba, partition.length);
        if(isoVFS->Initialize())
            vfs->Mount(isoVFS); //Mount the filesystem
    }
}