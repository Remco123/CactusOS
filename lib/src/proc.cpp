#include <proc.h>

using namespace LIBCactusOS;

int Process::Run(const char* path)
{
    return DoSyscall(SYSCALL_RUN_PROC, (uint32_t)path);
}
bool Process::CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len)
{
    return DoSyscall(SYSCALL_CREATE_SHARED_MEM, proc2ID, virtStart, len);
}