#include <core/gdt.h>

using namespace CactusOS::core;

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void* multiboot_structure, unsigned int multiboot_magic)
{
    GlobalDescriptorTable gdt;

    while(1);
}
