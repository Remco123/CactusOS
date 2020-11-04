#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__MOUSE_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__MOUSE_H

#include <system/drivers/usb/usbdriver.h>
#include <system/usb/hidparser.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            class USBMouse : public USBDriver
            {
            private:
                HIDParser hidParser;
                bool GetHIDProperty(struct HID_DATA* target, uint8_t* buffer, int bufLen, HID_USAGE item);

                bool useCustomReport = false;
                struct HID_DATA hidX;
                struct HID_DATA hidY;
                struct HID_DATA hidZ;

                int InInterruptEndpoint = -1;
            public:
                // Instance initializer
                USBMouse(USBDevice* dev);
                
                // Called when device is plugged into system
                bool Initialize() override;

                // Called when device is unplugged from system
                void DeInitialize() override;

                // Called by USB driver when we receive a interrupt packet
                bool HandleInterruptPacket(InterruptTransfer_t* transfer) override;
            };
        }
    }
}

#endif