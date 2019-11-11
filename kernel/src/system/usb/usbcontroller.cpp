#include <system/usb/usbcontroller.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBController::USBController(USBControllerType usbType)
{
    this->type = usbType;
}

void USBController::Setup()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}

bool USBController::ResetPort(uint8_t port)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
bool USBController::GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
bool USBController::GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}