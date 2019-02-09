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

        struct Thread;

        struct Process
        {
            int id;
            ProcessState state;
            List<Thread*> Threads;
            common::uint32_t pageDirPhys;

            //For Debuging
            char fileName[32];
        };

        class ProcessHelper
        {
        private:
            ProcessHelper();
        public:
            static Process* Create(char* fileName, bool isKernel = false);
        };
    }
}

#endif