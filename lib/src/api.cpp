#include <api.h>

#include <syscall.h>

using namespace LIBCactusOS;

void API::Initialize()
{
    //Call kernel to set this process as a cactusos process
    DoSyscall(SYSCALL_SET_CACTUSOS_LIB);
}