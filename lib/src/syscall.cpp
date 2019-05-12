#include <syscall.h>

/*
+---+--------------------+
| r |    Register(s)     |
+---+--------------------+
| a |   %eax, %ax, %al   |
| b |   %ebx, %bx, %bl   |
| c |   %ecx, %cx, %cl   |
| d |   %edx, %dx, %dl   |
| S |   %esi, %si        |
| D |   %edi, %di        |
+---+--------------------+
*/

int LIBCactusOS::DoSyscall(unsigned int intNum, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, unsigned int arg5)
{
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (intNum), "b" ((int)arg1), "c" ((int)arg2), "d" ((int)arg3), "S" ((int)arg4), "D" ((int)arg5));
    return a;
}