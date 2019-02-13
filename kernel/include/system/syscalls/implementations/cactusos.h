#ifndef __CACTUSOS__SYSTEM__SYSCALLS__IMPLEMENTATIONS_CACTUSOS_H
#define __CACTUSOS__SYSTEM__SYSCALLS__IMPLEMENTATIONS_CACTUSOS_H

#include <core/registers.h>

namespace CactusOS
{
    namespace system
    {
        class CactusOSSyscalls
        {
        public:
            static core::CPUState* HandleSyscall(core::CPUState* state);
        };
    }
}

#endif