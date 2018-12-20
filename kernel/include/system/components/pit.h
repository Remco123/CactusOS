#ifndef __CACTUSOS__CORE__PIT_H
#define __CACTUSOS__CORE__PIT_H

#include <system/interruptmanager.h>
#include <system/components/systemcomponent.h>
#include <core/port.h>

namespace CactusOS
{
    namespace system
    {
        #define PIT_FREQUENCY 1000

        class PIT : public SystemComponent, public InterruptHandler
        {
        private:
            volatile common::uint64_t timer_ticks;
        public:
            PIT();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void Sleep(common::uint32_t ms);

            //PCSpeaker
            void PlaySound(common::uint32_t nFrequence);
            void NoSound();
            void Beep();
            void Beep(common::uint32_t freq);
            void Beep(common::uint32_t freq, common::uint32_t duration);

            common::uint64_t Ticks();
        };
    }
}

#endif