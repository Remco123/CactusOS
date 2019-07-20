#include <system/disks/diskcontroller.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

DiskController::DiskController()
{ }
char DiskController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{ return 1; } //Needs to be implemented by driver
char DiskController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{ return 1; } //Needs to be implemented by driver
bool DiskController::EjectDrive(uint8_t drive)
{ return false; } //Needs to be implemented by driver