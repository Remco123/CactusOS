#include <system/drivers/usb/usbdriver.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBDriver::USBDriver(USBDevice* dev, char* driverName)
: Driver(driverName)
{
    this->device = dev;
}

USBDriver::~USBDriver()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}

void USBDriver::DeInitialize()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}

bool USBDriver::HandleInterruptPacket(InterruptTransfer_t* transfer)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}