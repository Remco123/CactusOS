#include <system/disks/diskmanager.h>

#include <system/drivers/disk/ide.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

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
    allDisks.push_back(disk);
}