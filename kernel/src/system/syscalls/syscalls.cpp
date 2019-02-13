#include <system/syscalls/syscalls.h>

#include <system/system.h>
#include <system/syscalls/implementations/cactusos.h>
#include <system/syscalls/implementations/linux.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

SystemCallHandler::SystemCallHandler()
: InterruptHandler(0x80) {

}

uint32_t SystemCallHandler::HandleInterrupt(uint32_t esp)
{
    int ID = System::scheduler->CurrentProcess()->syscallID;

    BootConsole::Write("Got syscall "); BootConsole::Write(Convert::IntToString(((CPUState*)esp)->EAX)); BootConsole::Write(" from process with id: "); BootConsole::WriteLine(Convert::IntToString(System::scheduler->CurrentProcess()->id));

    switch (ID)
    {
        case 0: //Linux Systemcall
            return (uint32_t)LinuxSyscalls::HandleSyscall((CPUState*)esp);
            break;
        case 1: //CactusOS Systemcall
            return (uint32_t)CactusOSSyscalls::HandleSyscall((CPUState*)esp);
            break;
            
        default:
            break;
    }

    return esp;
}