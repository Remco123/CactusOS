#include <system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);

Core* System::core = 0;

void System::InitCore(multiboot_info_t* mbi)
{
    printf("Starting Core\n");
    printf("CMD Parameters: "); printf((char*)mbi->cmdline); printf("\n");

    uint32_t* memupper = (uint32_t*)(((uint32_t)mbi) + 8);
    uint32_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    printf("Memory Manager Loaded\n");
    System::core->gdt = new GlobalDescriptorTable();
    printf("GDT Loaded\n");
    System::core->interrupts = new InterruptManager(0x20, System::core->gdt);
    printf("Interrupts Loaded\n");
    System::core->pit = new PIT(System::core->interrupts);
    printf("PIT Loaded\n");
}