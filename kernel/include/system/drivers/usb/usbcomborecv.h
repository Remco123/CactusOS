#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__COMBORECEIVER_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__COMBORECEIVER_H

#include <system/drivers/usb/usbdriver.h>
#include <system/usb/hidparser.h>
#include <system/input/keyboard.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            // Class describing a device used for both keyboard and mouse input
            class USBComboReceiver: public USBDriver, public Keyboard
            {
            private:
                uint8_t prevPacket[8];

                int keyboardIntEndpoint = -1;
                int mouseIntEndpoint = -1;

                int keyboardInterface = -1;
                int mouseInterface = -1;
            public:
                // Instance initializer
                USBComboReceiver(USBDevice* dev);
                
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