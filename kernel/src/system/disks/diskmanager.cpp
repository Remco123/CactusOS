#include <system/disks/diskmanager.h>

#include <system/drivers/disk/ide.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

// VM86 Function to get info about a specific disk device
// We must do it this way because the DS:SI gets used by the CallInterrupt() method
extern "C" uint8_t diskInfo;

DiskManager::DiskManager()
{ 
    allDisks.Clear();
}

char DiskManager::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    if(drive < allDisks.size())
        return allDisks[drive]->ReadSector(lba, buf);
    return 1;
}

char DiskManager::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    if(drive < allDisks.size())
        return allDisks[drive]->WriteSector(lba, buf);
    return 1;
}

void DiskManager::AddDisk(Disk* disk)
{
    //Add Disk to list of all disk
    allDisks.push_back(disk);
    //Try to detect filesystems present on disk
    PartitionManager::DetectAndLoadFilesystem(disk);
}
void DiskManager::RemoveDisk(Disk* disk)
{
    allDisks.Remove(disk); //Remove from list
    System::vfs->UnmountByDisk(disk); //And unmount all filesystems using that disk
}

BiosDriveParameters* DiskManager::GetDriveInfoBios(uint8_t drive)
{
    System::vm86Manager->ExecuteCode((uint32_t)&diskInfo, drive);
    return (BiosDriveParameters*)0x7000;
}