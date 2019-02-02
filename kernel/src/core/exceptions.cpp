#include <core/exceptions.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint32_t Exceptions::DivideByZero(uint32_t esp)
{
    BootConsole::ForegroundColor = VGA_COLOR_RED;
    BootConsole::WriteLine("Got Divide by zero Exception");

    InterruptDescriptorTable::DisableInterrupts();
    while(1);
}
uint32_t Exceptions::GeneralProtectionFault(uint32_t esp)
{
    BootConsole::ForegroundColor = VGA_COLOR_RED;
    BootConsole::WriteLine("Got General Protection Fault Exception");

    CPUState* state = (CPUState*)esp;

    if(state->ErrorCode != 0)
    {
        SelectorErrorCode* error = (SelectorErrorCode*)&state->ErrorCode;
        BootConsole::Write("External: "); BootConsole::WriteLine(Convert::IntToString(error->External));
        BootConsole::Write("Table: "); BootConsole::WriteLine(Convert::IntToString(error->Table));
        BootConsole::Write("Index: "); BootConsole::WriteLine(Convert::IntToString(error->TableIndex));
    }

    InterruptDescriptorTable::DisableInterrupts();
    while(1);
}
uint32_t Exceptions::PageFault(uint32_t esp)
{
    BootConsole::ForegroundColor = VGA_COLOR_BROWN;

    InterruptDescriptorTable::DisableInterrupts();

    uint32_t errorAddress;
    asm volatile("mov %%cr2, %0" : "=r" (errorAddress));

    CPUState* regs = (CPUState*)esp;

    // The error code gives us details of what happened.
    int present   = !(regs->ErrorCode & 0x1); // Page not present
    int rw = regs->ErrorCode & 0x2;           // Write operation?
    int us = regs->ErrorCode & 0x4;           // Processor was in user-mode?
    int reserved = regs->ErrorCode & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs->ErrorCode & 0x10;          // Caused by an instruction fetch?

    BootConsole::Write("Got Page Fault (");

    if(present)
        BootConsole::Write("present ");
    if(rw)
        BootConsole::Write("read-only ");
    if(us)
        BootConsole::Write("user-mode ");
    if(reserved)
        BootConsole::Write("reserved ");
    if(id)
        BootConsole::Write("instruction fetch ");

    BootConsole::Write(") at 0x");
    Print::printfHex32(errorAddress);
    BootConsole::WriteLine();

    while(1);
}



uint32_t Exceptions::HandleException(uint32_t number, uint32_t esp)
{
    switch(number)
    {
        case 0:
            return DivideByZero(esp);
        case 0xD:
            return GeneralProtectionFault(esp);
        case 0xE:
            return PageFault(esp);
        default:
            {
                BootConsole::ForegroundColor = VGA_COLOR_RED;
                BootConsole::Write("Unhandled exception: "); BootConsole::WriteLine(Convert::IntToString(number));
                BootConsole::WriteLine("Halting System");

                asm ("cli");
                while(1);
            }
    }
    return esp;
}