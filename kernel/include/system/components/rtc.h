#ifndef __CACTUSOS__SYSTEM__COMPONENTS__RTC_H
#define __CACTUSOS__SYSTEM__COMPONENTS__RTC_H

#include <core/port.h>
#include <system/components/systemcomponent.h>

namespace CactusOS
{
    namespace system
    {
        #define CURRENT_YEAR 2020

        class RTC : public SystemComponent
        {
        private:
            bool UpdateInProgress();
            common::uint8_t ReadRegister(int reg);
        public:
            RTC();

            common::uint32_t GetSecond();
            common::uint32_t GetMinute();
            common::uint32_t GetHour();
            common::uint32_t GetDay();
            common::uint32_t GetMonth();
            common::uint32_t GetYear();
        };  
    }
}

#endif