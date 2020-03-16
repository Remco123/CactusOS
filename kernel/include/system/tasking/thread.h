#ifndef __CACTOSOS__SYSTEM__TASKING__THREAD_H
#define __CACTOSOS__SYSTEM__TASKING__THREAD_H

#include <system/tasking/process.h>
#include <common/list.h>
#include <common/types.h>
#include <core/registers.h>

namespace CactusOS
{
    namespace system
    {
        #define THREAD_STACK_SIZE 4_KB
        
        #define SEG_USER_DATA 0x23
        #define SEG_USER_CODE 0x1B
        #define SEG_KERNEL_DATA 0x10
        #define SEG_KERNEL_CODE 8

        enum ThreadState
        {
            Blocked,
            Ready,
            Stopped,
            Started
        };

        enum BlockedState
        {
            Unkown,
            SleepMS,
            ReceiveIPC
        };

        struct Process;

        struct Thread
        {
            Process* parent;
            common::uint8_t* stack;
            common::uint8_t* userStack;
            common::uint32_t userStackSize;
            ThreadState state;
            BlockedState blockedState;
            core::CPUState* regsPtr;
            
            common::uint32_t timeDelta;
            common::uint8_t* FPUBuffer;
        };

        class ThreadHelper
        {
        private:
            ThreadHelper();
        public:
            static Thread* CreateFromFunction(void (*entryPoint)(), bool isKernel = false, common::uint32_t flags = 0x202, Process* parent = 0);
            static void RemoveThread(Thread* thread);
        };
    }
}

#endif