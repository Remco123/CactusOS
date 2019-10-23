#include <system/usb/usbmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBManager::USBManager()
: controllerList()
{ }

void USBManager::AddController(USBController* c)
{
    controllerList.push_back(c);
}
void USBManager::RemoveController(USBController* c)
{
    controllerList.Remove(c);
}

void USBManager::SetupAll()
{
    for(USBController* c : controllerList)
        c->Setup();
}

