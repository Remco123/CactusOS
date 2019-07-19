#ifndef __CACTUSOS__CORE__POWER_H
#define __CACTUSOS__CORE__POWER_H

#include <core/port.h>

namespace CactusOS
{
    namespace core
    {
        class Power
        {
        public:
            static void Initialize();
            static void Reboot();
            static void Poweroff();
        };
    }
}

#endif