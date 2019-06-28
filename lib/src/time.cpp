#include <time.h>

#include <syscall.h>

using namespace LIBCactusOS;

void Time::Sleep(uint32_t ms)
{
    DoSyscall(SYSCALL_SLEEP_MS, ms);
}

uint64_t Time::Ticks()
{
    uint64_t r = 0;
    DoSyscall(SYSCALL_GET_TICKS, (uint32_t)&r);
    return r;
}