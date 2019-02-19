#include <syscall.h>

using namespace CactusOS::common;

int LIBCactusOS::DoSyscall(uint32_t intNum, uint32_t arg1, uint32_t arg2)
{
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (intNum), "b" ((int)arg1), "c" ((int)arg2));
    return a;
}