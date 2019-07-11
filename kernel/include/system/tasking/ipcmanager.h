#ifndef __CACTUSOS__SYSTEM__TASKING__IPCMANAGER_H
#define __CACTUSOS__SYSTEM__TASKING__IPCMANAGER_H

#include <core/registers.h>
#include <system/tasking/process.h>

namespace CactusOS
{
    namespace system
    {
        struct IPCReceiveDescriptor
        {
            // Which process is waiting for a message?
            Process* receivingProcess;
            // Which thread called receive and is currently blocked.
            Thread* receivingThread;
            // Do we need to receive the message from a specific process?
            int receiveFromPID;
            // Do we need to receive a specific type of message?
            int receiveType;
        };

        class IPCManager
        {
        public:
            static void Initialize();
            static void HandleSend(core::CPUState* state, Process* proc);
            static void HandleReceive(core::CPUState* state, Process* proc);
        };
    }
}

#endif