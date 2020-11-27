#include <core/exceptions.h>
#include <system/system.h>
#include <common/print.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

//Should we try to automaticly fix pagefaults?
bool autoFixPagefaults = false;

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
    BootConsole::Write("EIP: 0x");
    Print::printfHex32(state->EIP);
    BootConsole::WriteLine();

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
    BootConsole::Write(" EIP: 0x");
    Print::printfHex32(regs->EIP);

    #if USE_HEAP_MAGIC
    BootConsole::Write(" Memory Intact: ");
    if(KernelHeap::CheckForErrors())
        BootConsole::Write(" No ");
    else
        BootConsole::Write(" Yes ");
    #endif
    
    if(System::scheduler != 0 && System::scheduler->CurrentProcess() != 0 && System::scheduler->CurrentProcess()->fileName != 0) {
        BootConsole::Write(" Process: ");
        BootConsole::WriteLine(System::scheduler->CurrentProcess()->fileName);
    }

    if(autoFixPagefaults && present) //Identity map error address
    {
        void* ptr = (void*)errorAddress;
        VirtualMemoryManager::mapVirtualToPhysical(ptr, ptr, true, true); //Readonly is probably a good idea        
        
        BootConsole::WriteLine(" Fixed pagefault");
        return esp;
    }

    while(1);
}

uint32_t Exceptions::TrapException(uint32_t esp)
{
    BootConsole::WriteLine("Got Trap Exception");
    BootConsole::Write("Next Instruction at: 0x"); Print::printfHex32(((CPUState*)esp)->EIP); BootConsole::WriteLine();

    return esp;
}

uint32_t Exceptions::FloatingPointException(uint32_t esp)
{
    BootConsole::WriteLine("Got SIMD Floating-Point Exception");
    uint32_t mxcsr;
    asm volatile ("stmxcsr %0":"=m" (mxcsr));
    BootConsole::Write("mxcsr: 0b"); Print::printbits(mxcsr); BootConsole::WriteLine();
    BootConsole::Write("Instruction Pointer: 0x"); Print::printfHex32(((CPUState*)esp)->EIP); BootConsole::WriteLine();

    InterruptDescriptorTable::DisableInterrupts();
    while(1);
}
uint32_t Exceptions::StackSegmentFault(uint32_t esp)
{
    BootConsole::ForegroundColor = VGA_COLOR_RED;
    BootConsole::WriteLine("Got Stack Segment Fault Exception");

    CPUState* state = (CPUState*)esp;
    BootConsole::Write("Instruction Pointer: 0x"); Print::printfHex32(((CPUState*)esp)->EIP); BootConsole::WriteLine();

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
        case 0x1:
            return TrapException(esp);
        case 0xC:
            return StackSegmentFault(esp);
        case 0x13:
            return FloatingPointException(esp);
        default:
            {
                BootConsole::ForegroundColor = VGA_COLOR_RED;
                BootConsole::Write("Unhandled exception: "); BootConsole::WriteLine(Convert::IntToString(number));
                BootConsole::WriteLine("Halting System");
                BootConsole::Write("Instruction Pointer: 0x"); Print::printfHex32(((CPUState*)esp)->EIP); BootConsole::WriteLine();

                asm ("cli");
                while(1);
            }
    }
    return esp;
}

void Exceptions::EnablePagefaultAutoFix()
{
    autoFixPagefaults = true;
}
void Exceptions::DisablePagefaultAutoFix()
{
    autoFixPagefaults = false;
}