#include <../../lib/include/syscall.h>
#include <system/tasking/ipcmanager.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

List<IPCReceiveDescriptor>* receivingBlockedList;
void IPCManager::Initialize()
{
    receivingBlockedList = new List<IPCReceiveDescriptor>();
}

//Called from systemcalls when process tries to send a ipc message
void IPCManager::HandleSend(core::CPUState* state, Process* proc)
{
    //Log(Info, "IPC Send from process %s", proc->fileName);
    // Get message pointer from ebx register
    LIBCactusOS::IPCMessage* msg = (LIBCactusOS::IPCMessage*)state->EBX;
    
    //Check if the source is valid
    if (msg->source != proc->id) {
        state->EAX = SYSCALL_RET_ERROR;
        return;
    }

    // To which process is this message meant to be send to
    Process* target = ProcessHelper::ProcessById(msg->dest);
    if (target == 0) {
        state->EAX = SYSCALL_RET_ERROR;
        return;
    }

    //Add the message to the buffer of the target process
    target->ipcMessages.push_back(*msg);

    int i = 0;
    for(IPCReceiveDescriptor desc : *receivingBlockedList) {
        if(desc.receivingProcess == target && (desc.receiveFromPID == -1 ? true : desc.receiveFromPID == proc->id) && (desc.receiveType == -1 ? true : desc.receiveType == msg->type))
        {    
            Thread* receivingThread = desc.receivingThread;
            if(receivingThread == 0)
                continue;
            
            receivingBlockedList->Remove(i);
            System::scheduler->Unblock(receivingThread);
            break;
        }
        i++;
    }

    state->EAX = SYSCALL_RET_SUCCES;
}

//ebx = Message pointer
//Called from systemcalls when process tries to receive a ipc message
void IPCManager::HandleReceive(core::CPUState* state, Process* proc)
{
    //Log(Info, "IPC Receive from process %s", proc->fileName);
    int recvFrom = state->ECX;
    int* errRet = (int*)state->EDX;
    int type = state->ESI;

    //We need to block ourself if there are no messages at the moment
    if (proc->ipcMessages.size() <= 0) {
        //Generate IPC Receive Descriptor
        IPCReceiveDescriptor desc;

        //////////
        // Fill in variables
        //////////
        desc.receivingProcess = proc;
        desc.receivingThread = System::scheduler->CurrentThread();
        desc.receiveFromPID = recvFrom;
        desc.receiveType = type;

        //Add this thread to the list of blocked processes
        receivingBlockedList->push_back(desc);
        System::scheduler->Block(desc.receivingThread, BlockedState::ReceiveIPC);
    }

    //If we get here we are either unblocked or there was already a message ready to receive
    int messageIndex = 0;
    LIBCactusOS::IPCMessage message = proc->ipcMessages.GetAt(messageIndex);
    
    //Loop throug all the messages until we find a correct one.
    while ((message.dest != proc->id || (recvFrom == -1 ? false : recvFrom != message.source) || (type == -1 ? false : type != message.type)) && (messageIndex < proc->ipcMessages.size())) { //Is the message not for us or not from the correct source
        proc->ipcMessages.GetAt(++messageIndex);
    }

    //We did not find a message that is for us or has the right parameters
    if(messageIndex == proc->ipcMessages.size())
    {
        if (errRet != 0)
            *errRet = SYSCALL_RET_ERROR;
        return;
    }

    //Copy message
    LIBCactusOS::IPCMessage* targetMessage = (LIBCactusOS::IPCMessage*)state->EBX;
    MemoryOperations::memcpy(targetMessage, &message, sizeof(LIBCactusOS::IPCMessage));

    //Remove it from the list
    proc->ipcMessages.Remove(messageIndex);

    if (errRet != 0)
        *errRet = SYSCALL_RET_SUCCES;
}