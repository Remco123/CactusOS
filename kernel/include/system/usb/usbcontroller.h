#ifndef __CACTUSOS__SYSTEM__USB__USBCONTROLLER_H
#define __CACTUSOS__SYSTEM__USB__USBCONTROLLER_H

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>
#include <system/usb/usbdevice.h>

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

        class USBDevice;
        class USBController
        {            
        public:
            //What type of controller is this
            USBControllerType type;
            //Create new instance of USBController class
            USBController(USBControllerType usbType);

            //Setup this controller into the active state
            virtual void Setup();

            //Function that will get called on a periodic interval in which the controller can perform specific kinds of things.
            //One of these things is port connection change checking and updating devices if necessary.
            virtual void ControllerChecksThread();

            /////
            // USB Common functions per controller
            /////
            //Reset specific port on controller
            virtual bool ResetPort(uint8_t port);
            //Get Device descriptor of specific device
            virtual bool GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device);
            //Get String descriptor of specific device
            virtual bool GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang = 0);
            //Get String descriptor of specific device
            //Returns buffer with Configuration header and additional data            
            virtual uint8_t* GetConfigDescriptor(USBDevice* device);
            //Set configuration for device
            virtual bool SetConfiguration(USBDevice* device, uint8_t config);
        };
    }
}

#endif