#include <core/virtualmemory.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void VirtualMemoryManager::PrintPageDirectoryEntry(PageDirectoryEntry pde)
{
    BootConsole::Write("#- Present: "); BootConsole::WriteLine(Convert::IntToString(pde.present));
    BootConsole::Write("#- Read Write: "); BootConsole::WriteLine(Convert::IntToString(pde.readWrite));
    BootConsole::Write("#- User: "); BootConsole::WriteLine(Convert::IntToString(pde.isUser));
    BootConsole::Write("#- Size: "); BootConsole::WriteLine(Convert::IntToString(pde.pageSize));
    BootConsole::Write("#- Frame: 0x"); Print::printfHex32(pde.frame); BootConsole::WriteLine();
}
void VirtualMemoryManager::PrintPageTableEntry(PageTableEntry pte)
{
    BootConsole::Write("#- Present: "); BootConsole::WriteLine(Convert::IntToString(pte.present));
    BootConsole::Write("#- Read Write: "); BootConsole::WriteLine(Convert::IntToString(pte.readWrite));
    BootConsole::Write("#- User: "); BootConsole::WriteLine(Convert::IntToString(pte.isUser));
    BootConsole::Write("#- Frame: 0x"); Print::printfHex32(pte.frame); BootConsole::WriteLine();
}


void VirtualMemoryManager::Intialize()
{
    BootConsole::WriteLine("Intializing Paging");
    BootConsole::Write("Page directory virtual address: 0x"); Print::printfHex32((uint32_t)&BootPageDirectory); BootConsole::WriteLine();

    PageDirectory* pageDirectory = (PageDirectory*)&BootPageDirectory;
    PageDirectoryEntry kernelPDE = pageDirectory->entries[KERNEL_PTNUM];
    
    BootConsole::WriteLine("Current kernel Page Directory Entry");
    PrintPageDirectoryEntry(kernelPDE);

    //The last entry in the Page Directory is a pointer to itself
    PageDirectoryEntry lastPDE;
    lastPDE.frame = (uint32_t)&BootPageDirectory;
    lastPDE.pageSize = FOUR_KB;
    lastPDE.isUser = 0;
    lastPDE.readWrite = 1;
    lastPDE.present = 1;

    pageDirectory->entries[1023] = lastPDE;

    void* pageTableAddress = PhysicalMemoryManager::AllocateBlock();

    //Create the kernel memory entry
    PageDirectoryEntry newKernelPDE;
    newKernelPDE.frame = (uint32_t)pageTableAddress;
    newKernelPDE.pageSize = FOUR_KB;
    newKernelPDE.isUser = 0;
    newKernelPDE.readWrite = 1;
    newKernelPDE.present = 1;

    pageDirectory->entries[KERNEL_PTNUM] = newKernelPDE;

    BootConsole::WriteLine("New kernel Page Directory Entry");
    PrintPageDirectoryEntry(newKernelPDE);

    BootConsole::WriteLine("Maping first 4mb of kernel to first 4mb of physical memory");

    //Map kernel's 4mb to first 4mb of physical memory
    PageTable* pageTable = (PageTable*)(phys2virt(pageDirectory->entries[KERNEL_PTNUM].frame));
    for(uint16_t i = 0; i < 1024; i++)
    {
        void* phys = (void*)(i * PAGE_SIZE);
        
        //Create page table entry
        PageTableEntry pte;
        pte.frame = (uint32_t)phys;
        pte.isUser = 0;
        pte.readWrite = 1;
        pte.present = 1;

        //And load it into the page table
        pageTable->entries[i] = pte;
    }

}