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
            struct KeyboardInternalStatus
            {
                bool LeftShift;
                bool RightShift;
                bool Alt;
                bool LeftControl;
                bool RightControl;
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