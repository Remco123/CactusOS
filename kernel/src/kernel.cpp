#include <multiboot/multiboot.h>
#include <core/gdt.h>
#include <core/tss.h>
#include <core/idt.h>
#include <core/physicalmemory.h>
#include <system/bootconsole.h>
#include <common/convert.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

extern uint32_t _kernel_end;

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const multiboot_info_t* mbi, unsigned int multiboot_magic)
{
    BootConsole::Init(true);
    BootConsole::ForegroundColor = VGA_COLOR_GREEN;
    BootConsole::BackgroundColor = VGA_COLOR_LIGHT_GREY;
    BootConsole::Clear();

    BootConsole::WriteLine("Starting Kernel");

    BootConsole::Write("CMD Line: "); BootConsole::WriteLine((char*)phys2virt(mbi->cmdline));

    GlobalDescriptorTable::Init();
    BootConsole::WriteLine("GDT Loaded");

    TSS::Install(5, 0x10, 0);
    BootConsole::WriteLine("TSS Loaded");

    InterruptManager::Install();
    BootConsole::WriteLine("Interrupts Loaded");

    PhysicalMemoryManager::Initialize(mbi->mem_upper * 1024, 0xC0000000 + _kernel_end);
    BootConsole::WriteLine("Physical Memory Loaded");

    PhysicalMemoryManager::ParseMemoryMap(mbi);
    PhysicalMemoryManager::SetRegionUsed(0x100000, _kernel_end); //mark kernel as used memory

    void* test = PhysicalMemoryManager::AllocateBlock();
    BootConsole::Write("Test Alloc: "); BootConsole::WriteLine(Convert::IntToString((int)test));

    InterruptManager::Enable();


    while(1);
}