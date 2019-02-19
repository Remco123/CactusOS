#include <log.h>
#include <syscall.h>

using namespace LIBCactusOS;

void LIBCactusOS::Log(LogLevel level, char* msg)
{
    DoSyscall(SYSCALL_LOG, level, (int)msg);
}