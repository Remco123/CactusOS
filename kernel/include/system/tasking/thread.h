#ifndef __CACTUSOS__SYSTEM__TASKING__THREAD_H
#define __CACTUSOS__SYSTEM__TASKING__THREAD_H

#include <common/types.h>
#include <core/idt.h>
#include <core/registers.h>
#include <system/interruptmanager.h>

namespace CactusOS
{
    namespace system
    {
        #define USER_DATA 0x23
        #define USER_CODE 0x1B
        #define KERNEL_DATA 0x10
        #define KERNEL_CODE 8

        class Thread
        {
        friend class Scheduler;
        protected:
            common::uint8_t stack[4096];
            core::CPUState* cpuState;
        public:
            static Thread* Create(void (*entryPoint)(), bool kernel);
        };
    }
}

#endif