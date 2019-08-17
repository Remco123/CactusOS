#ifndef __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H

#include <system/drivers/driver.h>
#include <system/interruptmanager.h>
#include <system/memory/fifostream.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define LEFT_SHIFT      0x2A
            #define RIGHT_SHIFT     0x36
            #define CAPS_LOCK       0x3A
            #define NUM_LOCK        0x45
            #define NUM_LOCK_LED    2
            #define CAPS_LOCK_LED   4

            struct KeyboardInternalStatus
            {
                bool LeftShift;
                bool RightShift;
                bool Alt;
                bool Control;
                bool CapsLock;
                bool NumberLock;
            };

            class KeyboardDriver : public InterruptHandler, public Driver, public FIFOStream
            {
            private:
                KeyboardInternalStatus status;
            public:
                KeyboardDriver();

                bool Initialize();
                void UpdateLeds();
                common::uint32_t HandleInterrupt(common::uint32_t esp);
            };
        }
    }
}

#endif