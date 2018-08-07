#ifndef __CACTUSOS__CORE__RTC_H
#define __CACTUSOS__CORE__RTC_H

#define CURRENT_YEAR 2018

#include <common/types.h>
#include <core/port.h>

namespace CactusOS
{
    namespace core
    {
        class RTC
        {
        protected:
            static bool get_update_in_progress_flag();
            static common::uint8_t get_RTC_register(int reg);
        public:
            static common::uint32_t GetSecond();
            static common::uint32_t GetMinute();
            static common::uint32_t GetHour();
            static common::uint32_t GetDay();
            static common::uint32_t GetMonth();
            static common::uint32_t GetYear();

            static void SleepSeconds(int secs); //Use this for only seconds when the pit is not loaded or interrupts are deactivated
        };
    }
}

#endif