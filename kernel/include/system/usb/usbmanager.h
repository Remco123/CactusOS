#ifndef __CACTUSOS__SYSTEM__USB__USBMANAGER_H
#define __CACTUSOS__SYSTEM__USB__USBMANAGER_H

#include <system/usb/usbcontroller.h>

namespace CactusOS
{
    namespace system
    {
        class USBManager
        { 
        private:
            //The list of all usb controllers present on this pc. (the ones detected by pci)
            List<USBController*> controllerList;
        public:
            //Create new instance of USBManager
            USBManager();

            //Add controller to list, called by HC drivers.
            void AddController(USBController* c);
            //Remove controller from list
            void RemoveController(USBController* c);
            //Send the setup command to all the controllers
            void SetupAll();
        };
    }
}

#endif