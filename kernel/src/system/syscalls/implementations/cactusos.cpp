#include <system/syscalls/implementations/cactusos.h>

#include <../../lib/include/syscall.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

CPUState* CactusOSSyscalls::HandleSyscall(CPUState* state)
{
    uint32_t sysCall = state->EAX;
    Process* proc = System::scheduler->CurrentProcess();

    switch (sysCall)
    {
        case SYSCALL_EXIT:
            Log(Info, "Process %d (%s) exited with code %d", proc->id, proc->fileName, (int)state->EBX);
            ProcessHelper::RemoveProcess(proc);
            break;
        case SYSCALL_LOG:
            Log((LogLevel)state->EBX, (const char* __restrict__)state->ECX);
            break;
    
        default:
            Log(Warning, "Got not supported syscall %d from process %d", sysCall, proc->id);
            break;
    }

    return state;
}