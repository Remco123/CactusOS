#ifndef __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__KEYBOARD_H

#include <system/drivers/driver.h>
#include <system/interruptmanager.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            class KeyboardDriver : public InterruptHandler, public Driver
            {
            private:
            public:
                KeyboardDriver();

                bool Initialize();
                common::uint32_t HandleInterrupt(common::uint32_t esp);
            };
        }
    }
}

#endif