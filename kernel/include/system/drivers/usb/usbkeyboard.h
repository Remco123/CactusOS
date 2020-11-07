#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__KEYBOARD_H

#include <system/drivers/usb/usbdriver.h>
#include <system/usb/hidparser.h>
#include <system/input/keyboard.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            class USBKeyboard : public USBDriver, public Keyboard
            {
            private:
                int InInterruptEndpoint = -1;

                uint8_t prevPacket[8];
            public:
                // Instance initializer
                USBKeyboard(USBDevice* dev);
                
                // Called when mass storage device is plugged into system
                bool Initialize() override;

                // Called when mass storage device is unplugged from system
                void DeInitialize() override;

                // Called by USB driver when we receive a interrupt packet
                bool HandleInterruptPacket(InterruptTransfer_t* transfer) override;

                // Update LED's on a keyboard device
                void UpdateLEDS() override;
            };
        }
    }
}

#endif