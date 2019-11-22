#include <system/usb/usbmanager.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBManager::USBManager()
: controllerList(), deviceList()
{ }

void USBManager::AddController(USBController* c)
{
    controllerList.push_back(c);
}
void USBManager::RemoveController(USBController* c)
{
    controllerList.Remove(c);
}
void USBManager::AddDevice(USBDevice* c)
{
    deviceList.push_back(c);
}
void USBManager::RemoveDevice(USBDevice* c)
{
    deviceList.Remove(c);
}

void USBManager::SetupAll()
{
    for(USBController* c : controllerList)
        c->Setup();
}

void USBManager::AssignAllDrivers()
{
    for(USBDevice* c : deviceList) {
        if(c->AssignDriver())
            Log(Info, "USBDevice %s driver assignment succes!", c->deviceName != 0 ? c->deviceName : "Unnamed");
        else
            Log(Warning, "USBDevice %s driver assignment failed!", c->deviceName != 0 ? c->deviceName : "Unnamed");
    }
}

