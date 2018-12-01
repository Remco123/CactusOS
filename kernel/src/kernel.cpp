#include <multiboot/multiboot.h>
#include <core/gdt.h>
#include <core/tss.h>
#include <core/idt.h>
#include <system/bootconsole.h>
#include <common/convert.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

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

    GlobalDescriptorTable::Init();
    BootConsole::WriteLine("GDT Loaded");

    TSS::Install(5, 0x10, 0);
    BootConsole::WriteLine("TSS Loaded");

    InterruptManager::Install();
    BootConsole::WriteLine("Interrupts Loaded");

    InterruptManager::Enable();


    while(1);
}