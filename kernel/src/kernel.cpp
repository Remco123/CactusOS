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

extern "C" uint32_t _kernel_base;
extern "C" uint32_t _kernel_end;

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
    //Basic kernel information gathered from the linker script
    uint32_t kernel_base = (uint32_t) &_kernel_base;
    uint32_t kernel_end = (uint32_t) &_kernel_end;
    uint32_t kernel_size = kernel_end - kernel_base;

    BootConsole::Init(true);
    BootConsole::ForegroundColor = VGA_COLOR_BLUE;
    BootConsole::BackgroundColor = VGA_COLOR_LIGHT_GREY;
    BootConsole::Clear();

    BootConsole::WriteLine("Starting Kernel");
    BootConsole::Write("Built on: "); BootConsole::WriteLine(__DATE__ "  " __TIME__);

    BootConsole::Write("Kernel Base: 0x"); Print::printfHex32(kernel_base); BootConsole::WriteLine();
    BootConsole::Write("Kernel End: 0x"); Print::printfHex32(kernel_end); BootConsole::WriteLine();
    BootConsole::Write("Kernel Size: "); BootConsole::Write(Convert::IntToString(kernel_size / 1024)); BootConsole::Write(" Kb      ("); BootConsole::Write(Convert::IntToString(kernel_size)); BootConsole::WriteLine(")");

    BootConsole::Write("GRUB Command Line Arguments: ");
    BootConsole::WriteLine((char*)phys2virt(mbi->cmdline));

    GlobalDescriptorTable::Init();
    BootConsole::WriteLine("GDT Loaded");

    TSS::Install(5, 0x10, 0);
    BootConsole::WriteLine("TSS Loaded");

    InterruptManager::Install();
    BootConsole::WriteLine("Interrupts Loaded");

    PhysicalMemoryManager::Initialize(mbi->mem_upper * 1024, kernel_end);
    BootConsole::WriteLine("Physical Memory Loaded");

    PhysicalMemoryManager::ParseMemoryMap(mbi);
    PhysicalMemoryManager::SetRegionUsed(0x100000, kernel_size); //mark physical kernel as used memory
    PhysicalMemoryManager::SetRegionUsed(0x0, 0x10000); //Protect the first 1mb of physical memory

    InterruptManager::Enable();
    BootConsole::WriteLine("Interrupts Enabled");

    while(1);
}