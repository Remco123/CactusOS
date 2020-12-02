#ifndef __CACTUSOS__SYSTEM__USB__USBMANAGER_H
#define __CACTUSOS__SYSTEM__USB__USBMANAGER_H

#include <system/usb/usbcontroller.h>
#include <system/usb/usbdevice.h>

namespace CactusOS
{
    namespace system
    {
        class USBManager
        { 
        public:
            // The list of all usb controllers present on this pc. (the ones detected by pci)
            List<USBController*> controllerList;
            // List of all known USBDevices
            List<USBDevice*> deviceList;
            // Holds if usb devices present on boot are all initialized
            bool initDone = false;
        public:
            //Create new instance of USBManager
            USBManager();

            //Add controller to list, called by HC drivers.
            void AddController(USBController* c);
            //Remove controller from list
            void RemoveController(USBController* c);
            //Add device to list, called by HC drivers.
            void AddDevice(USBDevice* c);
            //Remove device, called after device unplug
            void RemoveDevice(USBController* controller, uint8_t port);
            //Send the setup command to all the controllers
            void SetupAll();
            //Make all usb devices detect their properties and automaticly select a driver
            void AssignAllDrivers();
            //Check for changes in usb status
            void USBPoll();
        };
    }
}

#endif