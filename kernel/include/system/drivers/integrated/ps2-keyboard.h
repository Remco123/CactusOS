#ifndef __CACTUSOS__SYSTEM__DRIVERS__PS2KEYBOARD_H
#define __CACTUSOS__SYSTEM__DRIVERS__PS2KEYBOARD_H

#include <system/drivers/driver.h>
#include <system/interruptmanager.h>
#include <system/memory/fifostream.h>
#include <system/input/keyboard.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            class PS2KeyboardDriver : public InterruptHandler, public Keyboard, public Driver, public FIFOStream
            {
            public:
                PS2KeyboardDriver();

                bool Initialize();
                common::uint32_t HandleInterrupt(common::uint32_t esp);

                // Update LED's on a keyboard device
                void UpdateLEDS() override;
            };
        }
    }
}

#endif