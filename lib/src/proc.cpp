#include <proc.h>
#include <listing.h>

using namespace LIBCactusOS;

int Process::ID = -1;
int Process::numThreads = 1;
SharedSystemInfo* Process::systemInfo = 0;

int Process::Run(const char* path, bool block)
{
    return DoSyscall(SYSCALL_RUN_PROC, (uint32_t)path, (uint32_t)block);
}
bool Process::CreateSharedMemory(int proc2ID, uint32_t virtStart1, uint32_t virtStart2, uint32_t len)
{
    return DoSyscall(SYSCALL_CREATE_SHARED_MEM, proc2ID, virtStart1, virtStart2, len);
}
bool Process::CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len)
{
    return CreateSharedMemory(proc2ID, virtStart, virtStart, len);
}
bool Process::DeleteSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len)
{
    return DeleteSharedMemory(proc2ID, virtStart, virtStart, len);
}
bool Process::DeleteSharedMemory(int proc2ID, uint32_t virtStart1, uint32_t virtStart2, uint32_t len)
{
    return DoSyscall(SYSCALL_REMOVE_SHARED_MEM, proc2ID, virtStart1, virtStart2, len);
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
void Process::SetScheduler(bool active)
{
    DoSyscall(SYSCALL_SET_SCHEDULER, active);
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
int Process::StdInAvailable()
{
    return DoSyscall(SYSCALL_STDIO_AVAILABLE);
}
bool Process::Active(int pid)
{
    return DoSyscall(SYSCALL_PROC_EXIST, pid);
}
void Process::Unblock(int procPID, int thread)
{
    DoSyscall(SYSCALL_UNBLOCK, procPID, thread);
}