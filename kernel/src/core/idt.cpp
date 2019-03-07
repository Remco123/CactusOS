#include <core/idt.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

/*/////////////////
// Variables
/*/////////////////
IDTEntry idtEntries[IDT_ENTRY_SIZE];
IDTPointer idtPointer;


void InterruptDescriptorTable::SetDescriptor(uint32_t number, void (*handler)(), int accesLevel)
{
    uint32_t callerBase = (uint32_t)handler;

    idtEntries[number].handlerLowBits = (uint16_t)(callerBase & 0xFFFF);
    idtEntries[number].handlerHighBits = (uint16_t)((callerBase >> 16) & 0xFFFF);
    idtEntries[number].reserved = 0;
    idtEntries[number].access = IDT_PRESENT | ((accesLevel & 3) << 5) | IDT_INTERRUPT;
    idtEntries[number].selector = 0x8;
}

void InterruptDescriptorTable::Install()
{
    idtPointer.size = sizeof(IDTEntry) * IDT_ENTRY_SIZE - 1;
    idtPointer.base = (uint32_t)&idtEntries[0];

    MemoryOperations::memset((void*)&idtEntries[0], 0, sizeof(IDTEntry) * IDT_ENTRY_SIZE - 1);

    for(int i = 0; i < IDT_ENTRY_SIZE; i++)
        SetDescriptor(i, IgnoreInterrupt);

    SetDescriptor(0x00, HandleException0x00);
    SetDescriptor(0x01, HandleException0x01);
    SetDescriptor(0x02, HandleException0x02);
    SetDescriptor(0x03, HandleException0x03);
    SetDescriptor(0x04, HandleException0x04);
    SetDescriptor(0x05, HandleException0x05);
    SetDescriptor(0x06, HandleException0x06);
    SetDescriptor(0x07, HandleException0x07);
    SetDescriptor(0x08, HandleException0x08);
    SetDescriptor(0x09, HandleException0x09);
    SetDescriptor(0x0A, HandleException0x0A);
    SetDescriptor(0x0B, HandleException0x0B);
    SetDescriptor(0x0C, HandleException0x0C);
    SetDescriptor(0x0D, HandleException0x0D);
    SetDescriptor(0x0E, HandleException0x0E);
    SetDescriptor(0x0F, HandleException0x0F);
    SetDescriptor(0x10, HandleException0x10);
    SetDescriptor(0x11, HandleException0x11);
    SetDescriptor(0x12, HandleException0x12);
    SetDescriptor(0x13, HandleException0x13);

    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x00, HandleInterruptRequest0x00);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x01, HandleInterruptRequest0x01);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x02, HandleInterruptRequest0x02);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x03, HandleInterruptRequest0x03);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x04, HandleInterruptRequest0x04);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x05, HandleInterruptRequest0x05);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x06, HandleInterruptRequest0x06);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x07, HandleInterruptRequest0x07);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x08, HandleInterruptRequest0x08);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x09, HandleInterruptRequest0x09);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0A, HandleInterruptRequest0x0A);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0B, HandleInterruptRequest0x0B);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0C, HandleInterruptRequest0x0C);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0D, HandleInterruptRequest0x0D);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0E, HandleInterruptRequest0x0E);
    SetDescriptor(IDT_INTERRUPT_OFFSET + 0x0F, HandleInterruptRequest0x0F);

    SetDescriptor((IDT_INTERRUPT_OFFSET + 0xDD), HandleInterruptRequest0xDD);
    SetDescriptor(0x80, HandleInterruptRequest0x60, 3);

    // Remap the PIC
    outportb(0x20, 0x11);
    outportb(0xA0, 0x11);

    // remap
    outportb(0x21, IDT_INTERRUPT_OFFSET);
    outportb(0xA1, IDT_INTERRUPT_OFFSET+8);

    outportb(0x21, 0x04);
    outportb(0xA1, 0x02);

    outportb(0x21, 0x01);
    outportb(0xA1, 0x01);

    outportb(0x21, 0x00);
    outportb(0xA1, 0x00);

    asm volatile("lidt %0" : : "m" (idtPointer));
}
        

//Gets called for every interrupt from assembly code
uint32_t InterruptDescriptorTable::HandleInterrupt(CPUState* state)
{
    uint8_t interrupt = state->InterruptNumber;
    if(interrupt == 0xD && (state->EFLAGS & (1 << 17)))
    {
        //BootConsole::WriteLine("[IDT] VM86 Interrupt");
        //VM86 Interrupt
        state = (CPUState*)InterruptManager::HandleInterrupt(interrupt, (uint32_t)state);
    }
    else if(interrupt < IDT_INTERRUPT_OFFSET)
    {
        state = (CPUState*)Exceptions::HandleException(interrupt, (uint32_t)state);
    }
    else
    {
        //BootConsole::Write("Interrupt Handler for int: "); BootConsole::WriteLine(Convert::IntToString(interrupt));
        state = (CPUState*)InterruptManager::HandleInterrupt(interrupt, (uint32_t)state);
    }

    // hardware interrupts must be acknowledged
    if(IDT_INTERRUPT_OFFSET <= interrupt && interrupt < IDT_INTERRUPT_OFFSET+16)
    {
        outportb(0x20, 0x20);
        if(IDT_INTERRUPT_OFFSET + 8 <= interrupt)
            outportb(0xA0, 0x20);
    }

    return (uint32_t)state;
}

void InterruptDescriptorTable::EnableInterrupts()
{
    asm volatile ("sti");
}
void InterruptDescriptorTable::DisableInterrupts()
{
    asm volatile ("cli");
}
bool InterruptDescriptorTable::AreEnabled()
{
    unsigned long flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(flags) );
    return flags & (1 << 9);
}