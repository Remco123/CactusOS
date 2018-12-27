#ifndef __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MONITOR_H
#define __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MONITOR_H

#include <system/interruptmanager.h>
#include <core/registers.h>
#include <core/port.h>
#include <system/bootconsole.h>

namespace CactusOS
{
    namespace system
    {
        class Virtual8086Monitor : public InterruptHandler
        {
        public:
            Virtual8086Monitor();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
        };
    }
}

#endif