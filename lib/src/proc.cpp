#include <proc.h>

using namespace LIBCactusOS;

int Process::Run(const char* path)
{
    return DoSyscall(SYSCALL_RUN_PROC, (uint32_t)path);
}