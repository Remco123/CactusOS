#ifndef __CACTUSOS__SYSTEM__TASKING__PROCCES_H
#define __CACTUSOS__SYSTEM__TASKING__PROCCES_H

#include <common/types.h>
#include <core/idt.h>
#include <core/registers.h>
#include <system/interruptmanager.h>
#include <core/virtualmemory.h>

namespace CactusOS
{
    namespace system
    {
        #define USER_DATA 0x23
        #define USER_CODE 0x1B
        #define KERNEL_DATA 0x10
        #define KERNEL_CODE 8

        class Thread;

        class Procces
        {
        friend class Scheduler;
        protected:
            core::PageDirectory* pageDir; //The page directory the procces uses
            List<Thread*> Threads; //A list of pointers to the procces threads
        public:
            int ID; //Procces ID
            int syscallID; //Which syscall implementation should we call for this procces?
        };

        class Thread
        {
        friend class Scheduler;
        protected:
            Procces* parent; //The parent procces
            common::uint8_t* userStack; //Stack used by user
            common::uint8_t* kernelStack; //Stack used by kernel
            int state; //State of the thread
            core::CPUState cpuRegisters; //The saved register state of the thread
        public:
            static Thread* Create(void (*entryPoint)(), bool kernel);
        };
    }
}

#endif