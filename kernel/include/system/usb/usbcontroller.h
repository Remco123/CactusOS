#ifndef __CACTUSOS__SYSTEM__USB__USBCONTROLLER_H
#define __CACTUSOS__SYSTEM__USB__USBCONTROLLER_H

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>

namespace CactusOS
{
    namespace system
    {
        enum USBControllerType
        {
            UHCI,
            OHCI,
            EHCI,
            xHCI
        };

        class USBController
        {
        friend class USBManager;
        protected:
            USBControllerType type;
        public:
            USBController(USBControllerType usbType);

            virtual void Setup();
        };
    }
}

#endif