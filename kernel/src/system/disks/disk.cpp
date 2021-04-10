#include <system/disks/disk.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

Disk::Disk(uint32_t controllerIndex, DiskController* controller, DiskType type, uint64_t size, uint32_t blocks, uint32_t blocksize)
{
    this->controllerIndex = controllerIndex;
    this->controller = controller;
    this->type = type;
    this->size = size;
    this->blockSize = blocksize;
    this->numBlocks = blocks;
}
char Disk::ReadSector(uint32_t lba, uint8_t* buf)
{
    #if ENABLE_ADV_DEBUG
    System::statistics.diskReadOp += 1;
    #endif

    if(this->controller != 0)
        return this->controller->ReadSector(this->controllerIndex, lba, buf);
    return 1;
}
char Disk::WriteSector(uint32_t lba, uint8_t* buf)
{
    #if ENABLE_ADV_DEBUG
    System::statistics.diskWriteOp += 1;
    #endif

    if(this->controller != 0)
        return this->controller->WriteSector(this->controllerIndex, lba, buf);
    return 1;
}