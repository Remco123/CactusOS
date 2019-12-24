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
void USBController::ControllerChecksThread()
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
uint8_t* USBController::GetConfigDescriptor(USBDevice* device)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return 0;
}
bool USBController::SetConfiguration(USBDevice* device, uint8_t config)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
int USBController::GetMaxLuns(USBDevice* device)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return 0;
}
bool USBController::BulkIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
bool USBController::BulkOut(USBDevice* device, void* sendBuffer, int len, int endP)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}