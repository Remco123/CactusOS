#include <proc.h>

using namespace LIBCactusOS;

int Process::ID = -1;
int Process::numThreads = 1;
SharedSystemInfo* Process::systemInfo = 0;

int Process::Run(const char* path)
{
    return DoSyscall(SYSCALL_RUN_PROC, (uint32_t)path);
}
bool Process::CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t virtStart2, uint32_t len)
{
    return DoSyscall(SYSCALL_CREATE_SHARED_MEM, proc2ID, virtStart, virtStart2, len);
}
bool Process::CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len)
{
    return CreateSharedMemory(proc2ID, virtStart, virtStart, len);
}
void Process::CreateThread(void (*entryPoint)(), bool switchTo)
{
    DoSyscall(SYSCALL_START_THREAD, (uint32_t)entryPoint, switchTo);
    Process::numThreads++;
}
void Process::Yield()
{
    DoSyscall(SYSCALL_YIELD);
}
void Process::WriteStdOut(char byte)
{
    char bytes[1];
    bytes[0] = byte;
    DoSyscall(SYSCALL_WRITE_STDIO, (uint32_t)bytes, 1);
}
void Process::WriteStdOut(char* bytes, int length)
{
    DoSyscall(SYSCALL_WRITE_STDIO, (uint32_t)bytes, length);
}
char Process::ReadStdIn()
{
    return DoSyscall(SYSCALL_READ_STDIO);
}
void Process::BindSTDIO(int fromID, int toID)
{
    DoSyscall(SYSCALL_REDIRECT_STDIO, fromID, toID);
}