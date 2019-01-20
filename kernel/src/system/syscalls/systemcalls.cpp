#include <system/syscalls/systemcalls.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void SystemCallHandler::HandleSystemCall(CPUState* regs) { }

SystemCalls::SystemCalls()
: InterruptHandler(SYSCALL_VECT)
{
    this->sysCallsImplementations.Clear();
}

uint32_t SystemCalls::HandleInterrupt(uint32_t esp)
{
    CPUState* state = (CPUState*)esp;

    //TODO: Get the current procces syscall id and call the appropriate implementation
    /*
    int ID = GetCurrentProcces()->SycallsID;
    if(this->sysCallsImplementations[ID] != 0)
    {
        this->sysCallsImplementations[ID]->HandleSystemCall(state);
    }
    */

    return esp;
}

