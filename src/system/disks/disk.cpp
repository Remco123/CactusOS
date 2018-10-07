#include <system/disks/disk.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

Disk::Disk(uint32_t number, DiskController* controller, DiskType type)
{
    this->diskNumber = number;
    this->controller = controller;
    this->type = type;
}
char Disk::ReadSector(uint32_t lba, uint8_t* buf)
{
    if(this->controller != 0)
        return this->controller->ReadSector(this->diskNumber, lba, buf);
    return 1;
}
char Disk::WriteSector(uint32_t lba, uint8_t* buf)
{
    if(this->controller != 0)
        return this->controller->WriteSector(this->diskNumber, lba, buf);
    return 1;
}