#include <syscall.h>
#include <api.h>
#include <heap.h>
#include <gui/gui.h>
#include <log.h>

using namespace LIBCactusOS;

extern int main();

extern "C" void libMain()
{
    API::Initialize();

    UserHeap::Initialize();

    int ret = main();

    //GUI::CleanUp(); //TODO: Implement

    DoSyscall(SYSCALL_EXIT, ret);
}