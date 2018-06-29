#ifndef __CACTUSOS__CORE__PIT_H
#define __CACTUSOS__CORE__PIT_H

#include <core/interrupts.h>
#include <core/port.h>

namespace CactusOS
{
    namespace core
    {
        class PIT : public core::InterruptHandler
        {
        private:
            common::uint32_t timer_ticks;
        public:
            PIT(InterruptManager* interrupts);
            ~PIT();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void Sleep(common::uint32_t ms);

            //PCSpeaker
            void PlaySound(common::uint32_t nFrequence);
            void NoSound();

            void Beep();
            void Beep(common::uint32_t freq);
            void Beep(common::uint32_t freq, common::uint32_t duration);
        };
    }
}

#endif