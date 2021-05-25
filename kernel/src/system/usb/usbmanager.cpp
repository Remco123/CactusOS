#include <system/usb/usbmanager.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

extern char* USBControllerStrings[];

USBManager::USBManager()
: controllerList(), deviceList()
{
    this->initDone = false;
}

void USBManager::USBPoll()
{
    for(USBController* c : controllerList)
        c->ControllerChecksThread();
}

void USBManager::AddController(USBController* c)
{
    controllerList.push_front(c);
}
void USBManager::RemoveController(USBController* c)
{
    controllerList.Remove(c);
}
void USBManager::AddDevice(USBDevice* c)
{
    deviceList.push_back(c);

    if(this->initDone) {
        if(c->AssignDriver())
            Log(Info, "USBDevice %s driver assignment succes!", c->deviceName != 0 ? c->deviceName : "Unnamed");
        else
            Log(Warning, "USBDevice %s driver assignment failed!", c->deviceName != 0 ? c->deviceName : "Unnamed");
    }
}
void USBManager::RemoveDevice(USBController* controller, uint8_t port)
{
    USBDevice* dev = 0;
    for(USBDevice* c : deviceList)
        if(c->controller == controller && c->portNum == port) {
            dev = c;
            break;
        }
    
    if(dev == 0 || !initDone)
        Log(Error, "Device was removed from port but no USBDevice was found!");
    else
    {
        Log(Info, "Device %s Removed at port %d from %s controller", dev->deviceName != 0 ? dev->deviceName : "Unnamed", port, USBControllerStrings[dev->controller->type]);
        deviceList.Remove(dev);
        dev->OnUnplugged();
        delete dev;
    }
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
    this->initDone = true;
}

