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
            #define REQUEST_SET_IDLE 0x0A
            #define REQUEST_SET_PROTOCOL 0x0B

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
                //Instance initializer
                USBMouse(USBDevice* dev);
                
                //Called when mass storage device is plugged into system
                bool Initialize() override;

                //Called when mass storage device is unplugged from system
                void DeInitialize() override;
            };
        }
    }
}

#endif