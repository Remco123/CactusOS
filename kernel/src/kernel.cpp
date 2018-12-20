#include <multiboot/multiboot.h>
#include <core/gdt.h>
#include <core/tss.h>
#include <core/idt.h>
#include <core/physicalmemory.h>
#include <core/virtualmemory.h>
#include <system/bootconsole.h>
#include <system/memory/heap.h>
#include <system/memory/new.h>
#include <system/system.h>
#include <common/list.h>
#include <common/convert.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

extern "C" uint32_t _kernel_base;
extern "C" uint32_t _kernel_end;
extern "C" uint32_t _kernel_virtual_base;

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

    if(multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        BootConsole::WriteLine("Error: not booted by a multiboot bootloader");
        return;
    }

    BootConsole::WriteLine("Starting Kernel");
    BootConsole::Write("Built on: "); BootConsole::WriteLine(__DATE__ "  " __TIME__);

    BootConsole::Write("Kernel virtual base: 0x"); Print::printfHex32(_kernel_virtual_base); BootConsole::WriteLine();
    BootConsole::Write("Kernel Base: 0x"); Print::printfHex32(kernel_base); BootConsole::WriteLine();
    BootConsole::Write("Kernel End: 0x"); Print::printfHex32(kernel_end); BootConsole::WriteLine();
    BootConsole::Write("Kernel Size: "); BootConsole::Write(Convert::IntToString(kernel_size / 1024)); BootConsole::Write(" Kb      ("); BootConsole::Write(Convert::IntToString(kernel_size)); BootConsole::WriteLine(")");

    BootConsole::Write("GRUB Command Line Arguments: ");
    BootConsole::WriteLine((char*)phys2virt(mbi->cmdline));

    GlobalDescriptorTable::Init();
    BootConsole::WriteLine("GDT Loaded");

    TSS::Install(5, 0x10, 0);
    BootConsole::WriteLine("TSS Loaded");

    InterruptDescriptorTable::Install();
    BootConsole::WriteLine("IDT Loaded");

    PhysicalMemoryManager::Initialize(mbi->mem_upper * 1024, kernel_end);
    BootConsole::WriteLine("Physical Memory Loaded");

    //Parse the memory map handled by grub
    PhysicalMemoryManager::ParseMemoryMap(mbi);

    //Protect the first 1mb of physical memory + the end of kernel. 
    //We also add the size of the bitmap so that it does not have to be staticly allocated, this makes the kernel way smaller. 
    //Also round it to page bounds.
    PhysicalMemoryManager::SetRegionUsed(0x0, pageRoundUp(0x100000 + kernel_size + PhysicalMemoryManager::GetBitmapSize()));

    InterruptDescriptorTable::EnableInterrupts();
    BootConsole::WriteLine("Interrupts Enabled");

    VirtualMemoryManager::Intialize();
    BootConsole::WriteLine("Virtual Memory Loaded");

    KernelHeap::Initialize(KERNEL_HEAP_START, KERNEL_HEAP_START + KERNEL_HEAP_START_SIZE, KERNEL_HEAP_END);
    BootConsole::WriteLine("Kernel Heap Initialized");

    BootConsole::ForegroundColor = VGA_COLOR_MAGENTA;
    BootConsole::WriteLine("-Kernel core intialized-");

    //Further intialisation is done in the system class
    System::Start();

    while(1);
}