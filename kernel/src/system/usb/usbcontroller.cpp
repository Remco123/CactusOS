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