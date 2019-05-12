#include <../../lib/include/syscall.h>
#include <system/tasking/ipcmanager.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

/*
+-------+--------------------+
| state |    CPUState        |
+-------+--------------------+
| eax   |   Syscall num      |
| ebx   |   Message pointer  |
| ecx   |   Null             |
| edx   |   Null             |
| esi   |   Null             |
| edi   |   Null             |
+-------+--------------------+
*/
//Called from systemcalls when process tries to send a ipc message
void IPCManager::HandleSend(core::CPUState* state, Process* proc)
{
    LIBCactusOS::IPCMessage* msg = (LIBCactusOS::IPCMessage*)state->EBX;
    if (msg->source != proc->id || msg->dest == proc->id) {
        state->EAX = SYSCALL_RET_ERROR;
        return;
    }

    Process* target = ProcessHelper::ProcessById(msg->dest);
    if (target == 0) {
        state->EAX = SYSCALL_RET_ERROR;
        return;
    }

    //Add the message to the buffer of the target process
    target->ipcMessages.push_back(*msg);

    //Check if the target process is blocked and waiting for messages
    if(target->Threads[0]->state == ThreadState::Blocked && target->Threads[0]->blockedState == BlockedState::ReceiveIPC) //Change when process get multithreading
    {
        System::scheduler->Unblock(target->Threads[0]);
    }

    state->EAX = SYSCALL_RET_SUCCES;
}

/*
+-------+--------------------+
| state |    CPUState        |
+-------+--------------------+
| eax   |   Syscall num      |
| ebx   |   Message pointer  |
| ecx   |   From PID         |
| edx   |   Error out        |
| esi   |   Null             |
| edi   |   Null             |
+-------+--------------------+
*/
//Called from systemcalls when process tries to receive a ipc message
void IPCManager::HandleReceive(core::CPUState* state, Process* proc)
{
    int* errRet = (int*)state->EDX;
    int recvFrom = state->ECX;

    if (proc->ipcMessages.size() <= 0) { //We need to block ourself if there are no messages at the moment
        System::scheduler->Block(System::scheduler->CurrentThread(), BlockedState::ReceiveIPC);
    }

    LIBCactusOS::IPCMessage message = proc->ipcMessages.GetAt(0);
    if (message.dest != proc->id || (recvFrom == -1 ? false : recvFrom != message.source)) { //Is the message not for us or not from the correct source
        if (errRet != 0)
            *errRet = SYSCALL_RET_ERROR;
        return;
    }

    //Copy message
    LIBCactusOS::IPCMessage* targetMessage = (LIBCactusOS::IPCMessage*)state->EBX;
    MemoryOperations::memcpy(targetMessage, &message, sizeof(LIBCactusOS::IPCMessage));

    //Remove it from the list
    proc->ipcMessages.Remove(0);

    if (errRet != 0)
        *errRet = SYSCALL_RET_SUCCES;
}