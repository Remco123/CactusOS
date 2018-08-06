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
        class KeyboardDriver : public core::InterruptHandler, public Driver
        {
        private:
            char lastKey;
        public:
            bool keyAvailibe;
            
            KeyboardDriver(core::InterruptManager* interrupts);
            ~KeyboardDriver();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void Activate();

            char GetKey(bool wait = false);
        };
    }
}

#endif