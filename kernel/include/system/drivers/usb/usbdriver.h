#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__USBDRIVER_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__USBDRIVER_H

#include <system/drivers/driver.h>
#include <system/usb/usbdevice.h>

namespace CactusOS
{
    namespace system
    {
        typedef struct InterruptTransfer InterruptTransfer_t;

        class USBDriver : public drivers::Driver
        {
        public:
            // Which device is this driver for
            USBDevice* device;
        public:
            USBDriver(USBDevice* dev, char* driverName);
            virtual ~USBDriver();

            // De-Active this driver from the system
            // Called when device is unplugged
            virtual void DeInitialize();

            // Called from USB Controller when a interrupt packet is received
            virtual bool HandleInterruptPacket(InterruptTransfer_t* transfer);
        };
    }
}

#endif