#include <system/syscalls/syscalls.h>

#include <system/system.h>

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
            
            break;
        case 1: //CactusOS Systemcall

            break;    
            
        default:
            break;
    }

    return esp;
}