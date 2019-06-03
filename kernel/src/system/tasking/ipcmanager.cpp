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
    //Log(Info, "IPC Send from process %s", proc->fileName);
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
    //This is not working as it should but works for now
    //TODO: Really improve this!!!
    for(Thread* thread : target->Threads) {
        if(thread->state == ThreadState::Blocked && thread->blockedState == BlockedState::ReceiveIPC)
            System::scheduler->Unblock(thread);
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
| esi   |   type             |
| edi   |   Null             |
+-------+--------------------+
*/
//Called from systemcalls when process tries to receive a ipc message
void IPCManager::HandleReceive(core::CPUState* state, Process* proc)
{
    //Log(Info, "IPC Receive from process %s", proc->fileName);
    int* errRet = (int*)state->EDX;
    int recvFrom = state->ECX;
    int type = state->ESI;

    if (proc->ipcMessages.size() <= 0) { //We need to block ourself if there are no messages at the moment
        System::scheduler->Block(System::scheduler->CurrentThread(), BlockedState::ReceiveIPC);
    }

    LIBCactusOS::IPCMessage message = proc->ipcMessages.GetAt(0);
    if (message.dest != proc->id || (recvFrom == -1 ? false : recvFrom != message.source) || (type == -1 ? false : type != message.type)) { //Is the message not for us or not from the correct source
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