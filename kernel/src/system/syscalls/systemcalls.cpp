#include <system/syscalls/systemcalls.h>

#include <system/system.h>

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

    int ID = System::scheduler->GetCurrentProcces()->syscallID;
    if(this->sysCallsImplementations.size() > ID)
    {
        this->sysCallsImplementations[ID]->HandleSystemCall(state);
    }

    return (uint32_t)state;
}

