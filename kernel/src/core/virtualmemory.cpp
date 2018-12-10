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

    //Set all the entries before the kernel to 0
    for(int i = 0; i < KERNEL_PTNUM; i++)
        pageDirectory->entries[i] = {};


    PageDirectoryEntry kernelPDE = pageDirectory->entries[KERNEL_PTNUM];
    BootConsole::WriteLine("Current kernel Page Directory Entry");
    PrintPageDirectoryEntry(kernelPDE);



    //The last entry in the Page Directory is a pointer to itself
    PageDirectoryEntry lastPDE;
    MemoryOperations::memset(&lastPDE, 0, sizeof(PageDirectoryEntry));
    lastPDE.frame = virt2phys((uint32_t)&BootPageDirectory); //Convert it to physical address
    lastPDE.pageSize = FOUR_KB;
    lastPDE.isUser = 0;
    lastPDE.readWrite = 1;
    lastPDE.present = 1;
    pageDirectory->entries[1023] = lastPDE;



    //Create new page table by allocating a physical block of memory
    BootConsole::WriteLine("Creating new page table");
    void* pageTableAddress = PhysicalMemoryManager::AllocateBlock();
    MemoryOperations::memset(phys2virt(pageTableAddress), 0, sizeof(PageTable));


    //Create the kernel 4mb memory pde
    PageDirectoryEntry newKernelPDE;
    MemoryOperations::memset(&newKernelPDE, 0, sizeof(PageDirectoryEntry));
    newKernelPDE.frame = (uint32_t)pageTableAddress;
    newKernelPDE.pageSize = FOUR_KB;
    newKernelPDE.isUser = 0;
    newKernelPDE.readWrite = 1;
    newKernelPDE.present = 1;


    //First we need to fill in the kernel page table entries before asigning the new pde
    //Create the kernel 4mb memory page table
    PageTable* pageTable = (PageTable*)(phys2virt(pageTableAddress));
    MemoryOperations::memset(pageTable, 0, sizeof(PageTable));
    BootConsole::Write("4mb Page Table at: 0x"); Print::printfHex32((uint32_t)pageTable); BootConsole::WriteLine();

    //Fill in the 1024 pte's
    for(uint16_t i = 0; i < 1024; i++)
    {
        void* phys = (void*)(i * PAGE_SIZE);
        
        //Create page table entry
        PageTableEntry pte;
        MemoryOperations::memset(&pte, 0, sizeof(PageTableEntry));
        pte.frame = (uint32_t)phys;
        pte.isUser = 0;
        pte.readWrite = 1;
        pte.present = 1;

        //And load it into the page table
        pageTable->entries[i] = pte;
    }

    //And finally asign the new pde
    pageDirectory->entries[KERNEL_PTNUM] = newKernelPDE;

    BootConsole::WriteLine("New kernel Page Directory Entry");
    PrintPageDirectoryEntry(newKernelPDE);
}