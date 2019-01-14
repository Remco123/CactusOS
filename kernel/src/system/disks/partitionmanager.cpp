#include <system/disks/partitionmanager.h>

#include <system/vfs/iso9660.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

static uint8_t Readbuf[2048];

void PartitionManager::DetectAndLoadFilesystems(DiskManager* disks, VFSManager* vfs)
{
    BootConsole::WriteLine("Detecting partitions on disks");
    MemoryOperations::memset(Readbuf, 0, 2048);

    for(int i = 0; i < disks->allDisks.size(); i++)
    {
        char ret = disks->allDisks[i]->ReadSector(0, Readbuf);
        if(ret == 0)
        {
            MasterBootRecord* mbr = (MasterBootRecord*)Readbuf;
            if(mbr->magicnumber != 0xAA55)
            {
                BootConsole::WriteLine("MBR Magic Number is not correct");
                BootConsole::Write("Instead it was: "); Print::printfHex16(mbr->magicnumber); BootConsole::WriteLine();
                continue;
            }
            //Loop trough partitions
            for(int p = 0; p < 4; p++)
            {
                if(mbr->primaryPartitions[p].partition_id == 0x00)
                    continue;

                BootConsole::Write("- Disk "); BootConsole::Write(Convert::IntToString(i));                
                BootConsole::Write(" Partition: "); BootConsole::Write(Convert::IntToString(p));

                if(mbr->primaryPartitions[p].bootable == 0x80)
                    BootConsole::Write(" Bootable ID: 0x");
                else
                    BootConsole::Write(" ID: 0x");
                
                Print::printfHex(mbr->primaryPartitions[p].partition_id);
                BootConsole::Write(" Sectors: "); BootConsole::Write(Convert::IntToString(mbr->primaryPartitions[p].length));

                AssignVFS(mbr->primaryPartitions[p], disks->allDisks[i], vfs);

                BootConsole::WriteLine();
            }
        }
        else
        {
            BootConsole::Write("Error reading disk: "); BootConsole::Write(Convert::IntToString(i)); BootConsole::Write(" Code: 0x"); Print::printfHex(ret); BootConsole::WriteLine();
        }
    }
}

void PartitionManager::AssignVFS(PartitionTableEntry partition, Disk* disk, VFSManager* vfs)
{
    if(partition.partition_id == 0xCD)
    {
        BootConsole::Write(" [ISO9660]");
        ISO9660* isoVFS = new ISO9660(disk, partition.start_lba, partition.length);
        if(isoVFS->Initialize())
            vfs->Mount(isoVFS); //Mount the filesystem
        else
            delete isoVFS;
    }
}