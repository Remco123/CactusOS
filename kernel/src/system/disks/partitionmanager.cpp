#include <system/disks/partitionmanager.h>

#include <system/vfs/iso9660.h>
#include <system/vfs/fat32.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void PartitionManager::DetectAndLoadFilesystem(Disk* disk)
{
    char* diskIdentifier = (char*)(disk->identifier == 0 ? "" : disk->identifier);
    Log(Info, "Detecting partitions on disk %s", diskIdentifier);
    
    uint8_t* Readbuf = new uint8_t[CDROM_SECTOR_SIZE];
    MemoryOperations::memset(Readbuf, 0, CDROM_SECTOR_SIZE);

    char ret = disk->ReadSector(0, Readbuf);
    if(ret == 0)
    {
        MasterBootRecord* mbr = (MasterBootRecord*)Readbuf;
        if(mbr->magicnumber != 0xAA55) {
            Log(Warning, "MBR magic number is not 0xAA55 instead %w", mbr->magicnumber);
            delete Readbuf;
            return;
        }
        //Loop trough partitions
        for(int p = 0; p < 4; p++)
        {
            if(mbr->primaryPartitions[p].partition_id == 0x00)
                continue;

            BootConsole::Write("- Disk "); BootConsole::Write(diskIdentifier);                
            BootConsole::Write(" Partition: "); BootConsole::Write(Convert::IntToString(p));

            if(mbr->primaryPartitions[p].bootable == 0x80)
                BootConsole::Write(" Bootable ID: 0x");
            else
                BootConsole::Write(" ID: 0x");
            
            Print::printfHex(mbr->primaryPartitions[p].partition_id);
            BootConsole::Write(" Sectors: "); BootConsole::Write(Convert::IntToString(mbr->primaryPartitions[p].length));

            AssignVFS(mbr->primaryPartitions[p], disk);

            BootConsole::WriteLine();
        }
    }
    else {
        BootConsole::Write("Error reading disk "); BootConsole::Write(diskIdentifier); BootConsole::Write(" Code: 0x"); Print::printfHex(ret); BootConsole::WriteLine();
    }
    delete Readbuf;
}

void PartitionManager::AssignVFS(PartitionTableEntry partition, Disk* disk)
{
    if(partition.partition_id == 0xCD)
    {
        BootConsole::Write(" [ISO9660]");
        ISO9660* isoVFS = new ISO9660(disk, partition.start_lba, partition.length);
        if(isoVFS->Initialize())
            System::vfs->Mount(isoVFS); //Mount the filesystem
        else
            delete isoVFS;
    }
    else if(partition.partition_id == 0x0B || partition.partition_id == 0x0C)
    {
        BootConsole::Write(" [FAT32]");
        FAT32* isoVFS = new FAT32(disk, partition.start_lba, partition.length);
        if(isoVFS->Initialize())
            System::vfs->Mount(isoVFS); //Mount the filesystem
        else
            delete isoVFS;
    }
}