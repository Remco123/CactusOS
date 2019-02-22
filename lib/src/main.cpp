#include <syscall.h>

using namespace LIBCactusOS;

extern int main();

extern "C" void libMain()
{
    int ret = main();

    DoSyscall(SYSCALL_EXIT, ret);
}