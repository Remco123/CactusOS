#include <system/syscalls/implementations/cactusos.h>

#include <../../lib/include/syscall.h>

#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

CPUState* CactusOSSyscalls::HandleSyscall(CPUState* state)
{
    uint32_t sysCall = state->EAX;

    switch (sysCall)
    {
        case SYSCALL_LOG:
            Log((LogLevel)state->EBX, (char*)state->ECX);
            break;
    
        default:
            break;
    }

    return state;
}