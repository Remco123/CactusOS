#include <system/disks/diskcontroller.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

DiskController::DiskController()
{
    
}

char DiskController::ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf)
{ return 1; } //Needs to be implemented by driver
char DiskController::WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf)
{ return 1; } //Needs to be implemented by driver
void DiskController::AsignDisks(DiskManager* manager)
{ } //Needs to be implemented by driver