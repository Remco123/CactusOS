#ifndef __CACTUSOS__SYSTEM__SYSCALLS__SYSTEMCALLS_H
#define __CACTUSOS__SYSTEM__SYSCALLS__SYSTEMCALLS_H

#include <system/interruptmanager.h>
#include <core/registers.h>

namespace CactusOS
{
    namespace system
    {
        #define SYSCALL_VECT 0x80

        class SystemCallHandler
        {
        public:
            virtual void HandleSystemCall(core::CPUState* regs);
        };

        class SystemCalls : public InterruptHandler
        {
        private:
            List<SystemCallHandler*> sysCallsImplementations;
        public:
            SystemCalls();

            common::uint32_t HandleInterrupt(common::uint32_t esp);
        };
    }
}

#endif