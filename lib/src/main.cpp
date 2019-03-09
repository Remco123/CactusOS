#include <syscall.h>
#include <api.h>
#include <heap.h>

using namespace LIBCactusOS;

extern int main();

extern "C" void libMain()
{
    API::Initialize();

    UserHeap::Initialize();

    int ret = main();

    DoSyscall(SYSCALL_EXIT, ret);
}