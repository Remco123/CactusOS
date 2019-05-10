#include <syscall.h>

int LIBCactusOS::DoSyscall(unsigned int intNum, unsigned int arg1, unsigned int arg2, unsigned int arg3)
{
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (intNum), "b" ((int)arg1), "c" ((int)arg2), "d" ((int)arg3));
    return a;
}