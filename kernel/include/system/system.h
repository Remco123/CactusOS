#ifndef __CACTUSOS__SYSTEM__SYSTEM_H
#define __CACTUSOS__SYSTEM__SYSTEM_H

#include <system/bootconsole.h>
#include <system/components/systemcomponent.h>
#include <system/components/pit.h>
#include <system/components/rtc.h>
#include <system/components/smbios.h>

namespace CactusOS
{
    namespace system
    {
        class System
        {
        public:
            static PIT* pit;
            static RTC* rtc;
            static SMBIOS* smbios;

            static void Start();
        };
    }
} 

#endif