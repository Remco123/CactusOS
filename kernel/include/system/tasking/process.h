#ifndef __CACTOSOS__SYSTEM__TASKING__PROCESS_H
#define __CACTOSOS__SYSTEM__TASKING__PROCESS_H

#include <system/tasking/thread.h>
#include <common/list.h>
#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        enum ProcessState
        {
            Sleep,
            Active
        };

        #define PROC_USER_HEAP_SIZE 0x100000 //1 MB heap space for processes, and ofcourse more if needed.

        struct Thread;

        struct Process
        {
            int id;
            int syscallID;
            bool isUserspace;
            ProcessState state;
            List<Thread*> Threads;
            common::uint32_t pageDirPhys;
            struct Excecutable
            {
                common::uint32_t memBase;
                common::uint32_t memSize;
            } excecutable;
            
            struct Heap
            {
                common::uint32_t heapStart;
                common::uint32_t heapEnd;
            } heap;
            
            

            //For Debuging
            char fileName[32];
        };

        class ProcessHelper
        {
        private:
            ProcessHelper();
        public:
            static Process* Create(char* fileName, bool isKernel = false);
            static Process* CreateKernelProcess();
            static void RemoveProcess(Process* proc);
        };
    }
}

#endif