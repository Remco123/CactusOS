#include <syscall.h>
#include <api.h>
#include <heap.h>
#include <gui/gui.h>
#include <log.h>
#include <math.h>

using namespace LIBCactusOS;

extern int main();

typedef void (*constructor)();

extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++) {
        (*i)();
    }
}

extern "C" constructor start_dtors;
extern "C" constructor end_dtors;
extern "C" void callDestructors()
{
    for(constructor* i = &start_dtors; i != &end_dtors; i++) {
        (*i)();
    }
}

extern "C" void libMain()
{
    API::Initialize();

    Math::EnableFPU();

    UserHeap::Initialize();

    callConstructors();

    int ret = main();

    callDestructors();

    //GUI::CleanUp(); //TODO: Implement

    DoSyscall(SYSCALL_EXIT, ret);
}