#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__USBDRIVER_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__USBDRIVER_H

#include <system/drivers/driver.h>
#include <system/usb/usbdevice.h>

namespace CactusOS
{
    namespace system
    {
        class USBDriver : public drivers::Driver
        {
        protected:
            //Which device is this driver for
            USBDevice* device;
        public:
            USBDriver(USBDevice* dev, char* driverName);

            //De-Active this driver from the system
            //Called when device is unplugged
            virtual void DeInitialize();
        };
    }
}

#endif