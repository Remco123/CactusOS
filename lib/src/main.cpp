#include <syscall.h>
#include <api.h>
#include <heap.h>
#include <gui/gui.h>
#include <log.h>
#include <math.h>
#include <string.h>

using namespace LIBCactusOS;

// External pointer to main function, will be linked later by the compiler
// This is the part of the program where the user has control, this is more in the background
extern int main(int argc, char** argv);

// Create a easy usable type for constructors
typedef void (*constructor)();

// Defined in linker.ld
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;

// Needs to be callable from assembly, hence the extern "C"
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++) {
        (*i)();
    }
}

// TODO: Is this right? It does not seem to be defined anywhere
extern "C" constructor start_dtors;
extern "C" constructor end_dtors;

// Needs to be callable from assembly, hence the extern "C"
extern "C" void callDestructors()
{
    for(constructor* i = &start_dtors; i != &end_dtors; i++) {
        (*i)();
    }
}

// A function that takes a arguments string and parses it to the format that GCC and all normal programs accept
// For example:
// Input -> -a all -b extra ./hello
// Output -> argc = 5 argv = {"-a", "all", "-b", "extra", "./hello" }
char** ArgumentParser(char* arguments, int* argcPtr)
{
    List<char*> splitList = str_Split(arguments, ' ');
    *argcPtr = splitList.size();

    char** output = new char*[splitList.size()];
    for(int i = 0; i < splitList.size(); i++) {
        char* src = splitList[i];
        int srcLen = strlen(src);

        output[i] = new char[srcLen + 1];
        char* dst = output[i];
        memcpy(dst, src, srcLen);
        dst[srcLen] = '\0';
    }
    return output;
}

// Called by the initializing assembly (crt0.asm)
extern "C" void libMain()
{
    // First tell the systemcalls subsystem that we want to use CactusOS syscalls and not linux
    API::Initialize();

    // Enable the FPU device for mathematical calculations
    Math::EnableFPU();

    // We also need memory to work, this will make sure that new and delete actually can be used
    UserHeap::Initialize();

    // Prepare some stuff for the GUI. Yes this is also called for console applications since it is so common.
    GUI::Initialize();

    // Call all initializing constructors
    callConstructors();

    // Create a buffer for receiving the arguments from the kernel
    char* argBuffer = new char[PROC_ARG_LEN_MAX];
    memset(argBuffer, 0, PROC_ARG_LEN_MAX);

    // Request arguments from kernel
    DoSyscall(SYSCALL_GET_ARGUMENTS, (uint32_t)argBuffer);

    // Parse arguments into common format
    int argc = 0;
    char** argv = ArgumentParser(argBuffer, &argc);

    // Now we can call the user function main, this is where the program seems to start for the user
    int ret = main(argc, argv);

    // Perform some cleanups
    callDestructors();
    GUI::CleanUp();

    // And make sure this program actually quits
    DoSyscall(SYSCALL_EXIT, ret);

    // This is real bad, this means the scheduler is broken
    while(1) Log(Error, "CRITICAL: Program does not appear to be stopped!");
}