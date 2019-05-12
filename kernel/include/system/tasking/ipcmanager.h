#ifndef __CACTUSOS__SYSTEM__TASKING__IPCMANAGER_H
#define __CACTUSOS__SYSTEM__TASKING__IPCMANAGER_H

#include <core/registers.h>
#include <system/tasking/process.h>

namespace CactusOS
{
    namespace system
    {
        class IPCManager
        {
        public:
            static void HandleSend(core::CPUState* state, Process* proc);
            static void HandleReceive(core::CPUState* state, Process* proc);
        };
    }
}

#endif