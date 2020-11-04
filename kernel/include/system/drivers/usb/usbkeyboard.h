#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__KEYBOARD_H

#include <system/drivers/usb/usbdriver.h>
#include <system/usb/hidparser.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            struct USBKeyboardInternalStatus
            {
                bool LeftControl;
                bool LeftShift;
                bool LeftAlt;
                bool LeftGUI;
                bool RightControl;
                bool RightShift;
                bool RightAlt;
                bool RightGUI;
            };

            class USBKeyboard : public USBDriver
            {
            private:
                int InInterruptEndpoint = -1;
                USBKeyboardInternalStatus status;
            public:
                // Instance initializer
                USBKeyboard(USBDevice* dev);
                
                // Called when mass storage device is plugged into system
                bool Initialize() override;

                // Called when mass storage device is unplugged from system
                void DeInitialize() override;

                // Called by USB driver when we receive a interrupt packet
                bool HandleInterruptPacket(InterruptTransfer_t* transfer) override;
            };
        }
    }
}

#endif