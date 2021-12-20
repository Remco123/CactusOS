#ifndef __CACTOSOS__SYSTEM__TASKING__PROCESS_H
#define __CACTOSOS__SYSTEM__TASKING__PROCESS_H

#include <system/tasking/thread.h>
#include <common/list.h>
#include <common/types.h>
#include <../../lib/include/ipc.h>
#include <system/memory/stream.h>

namespace CactusOS
{
    namespace system
    {
        class SymbolDebugger;
        
        enum ProcessState
        {
            Active
        };

        #define PROC_USER_HEAP_SIZE 1_MB //1 MB heap space for processes, and of course more if needed.

        struct Thread;

        struct Process
        {
            int id;
            int syscallID;
            bool isUserspace;
            char* args;
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
            List<LIBCactusOS::IPCMessage> ipcMessages;

            Stream* stdInput;
            Stream* stdOutput;

            // For Debuging
            char fileName[32];

            // Debugger assigned to this process
            SymbolDebugger* symDebugger = 0;
        };

        class ProcessHelper
        {
        private:
            ProcessHelper();
        public:
            static List<Process*> Processes;
            
            static Process* Create(char* fileName, char* arguments = 0, bool isKernel = false);
            static Process* CreateKernelProcess();
            static void RemoveProcess(Process* proc);
            static void UpdateHeap(Process* proc, common::uint32_t newEndAddr);
            static Process* ProcessById(int id);
        };
    }
}

#endif