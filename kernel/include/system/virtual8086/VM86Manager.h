#ifndef __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MANAGER_H
#define __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MANAGER_H

#include <system/interruptmanager.h>
#include <system/virtual8086/VM86Args.h>
#include <core/registers.h>
#include <core/idt.h>
#include <core/tss.h>

namespace CactusOS
{
    namespace system
    {
        class Virtual8086Manager : public InterruptHandler
        {
        public:
            Virtual8086Manager();
            common::uint32_t HandleInterrupt(common::uint32_t esp);
            
            void CallInterrupt(common::uint8_t intNumber, VM86Arguments* regs);
        };
    }
}

#endif