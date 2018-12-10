#include <core/virtualmemory.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

PageDirectory* currentPageDirectory;

void VirtualMemoryManager::PrintPageDirectoryEntry(PageDirectoryEntry pde)
{
    BootConsole::Write("#- Present: "); BootConsole::WriteLine(Convert::IntToString(pde.present));
    BootConsole::Write("#- Read Write: "); BootConsole::WriteLine(Convert::IntToString(pde.readWrite));
    BootConsole::Write("#- User: "); BootConsole::WriteLine(Convert::IntToString(pde.isUser));
    BootConsole::Write("#- Size: "); BootConsole::WriteLine(pde.pageSize ? (char*)"4m" : (char*)"4k");
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

    currentPageDirectory = pageDirectory;
}