#ifndef __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H

#include <common/types.h>
#include <core/interrupts.h>
#include <system/drivers/driver.h>
#include <core/port.h>

namespace CactusOS
{
    namespace system
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

        class KeyboardDriver : public core::InterruptHandler, public Driver
        {
        private:
            char lastKey;
            KeyboardInternalStatus Status;
        public:
            bool keyAvailibe;
            
            KeyboardDriver(core::InterruptManager* interrupts);
            ~KeyboardDriver();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void Activate();

            char GetKey(bool wait = false);
            void SetLeds(common::uint8_t code);
        };
    }
}

#endif