#ifndef __CACTUSOS__SYSTEM__SYSCALLS__IMPLEMENTATIONS_LINUX_H
#define __CACTUSOS__SYSTEM__SYSCALLS__IMPLEMENTATIONS_LINUX_H

#include <core/registers.h>

namespace CactusOS
{
    namespace system
    {
        class LinuxSyscalls
        {
        public:
            static core::CPUState* HandleSyscall(core::CPUState* state);
        };
    }
}

#endif