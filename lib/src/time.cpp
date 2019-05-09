#include <time.h>

#include <syscall.h>

using namespace LIBCactusOS;

void Time::Sleep(uint32_t ms)
{
    DoSyscall(SYSCALL_SLEEP_MS, ms);
}