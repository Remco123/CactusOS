#include <core/virtualmemory.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

PageDirectory* VirtualMemoryManager::currentPageDirectory = 0;

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
void VirtualMemoryManager::ReloadCR3()
{
    asm volatile("movl	%cr3,%eax");
	asm volatile("movl	%eax,%cr3");
}


void VirtualMemoryManager::Intialize()
{
    BootConsole::WriteLine("Intializing Paging");
    BootConsole::Write("Page directory virtual address: 0x"); Print::printfHex32((uint32_t)&BootPageDirectory); BootConsole::WriteLine();
    
    uint32_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    BootConsole::Write("Current CR3 Value: 0x"); Print::printfHex32(cr3); BootConsole::WriteLine();

    //The Page Directory created by the loader
    PageDirectory* pageDirectory = (PageDirectory*)&BootPageDirectory;

    //The current 4mb PDE used by the kernel
    PageDirectoryEntry currentKernelPDE = pageDirectory->entries[KERNEL_PTNUM];
    BootConsole::WriteLine("Current kernel Page Directory Entry");
    PrintPageDirectoryEntry(currentKernelPDE);

    //Identity map first 4mb with a 4mb page
    PageDirectoryEntry first4MbPDE;
    MemoryOperations::memset(&first4MbPDE, 0, sizeof(PageDirectoryEntry));
    first4MbPDE.frame = 0;
    first4MbPDE.pageSize = FOUR_MB;
    first4MbPDE.isUser = 0;
    first4MbPDE.readWrite = 1;
    first4MbPDE.present = 1;
    pageDirectory->entries[0] = first4MbPDE;

    //The last entry in the Page Directory is a pointer to itself
    PageDirectoryEntry lastPDE;
    MemoryOperations::memset(&lastPDE, 0, sizeof(PageDirectoryEntry));
    lastPDE.frame = virt2phys((uint32_t)&BootPageDirectory); //Convert it to physical address
    lastPDE.pageSize = FOUR_KB;
    lastPDE.isUser = 0;
    lastPDE.readWrite = 1;
    lastPDE.present = 1;
    pageDirectory->entries[1023] = lastPDE;



    //Here we map the first 4mb of the kernel to the first 4mb of physical memory
    void* kernelPageTableAddress = PhysicalMemoryManager::AllocateBlock(); //The physical address of the new page table

    //The kernel page directory entry
    PageDirectoryEntry kernelPDE;
    MemoryOperations::memset(&kernelPDE, 0, sizeof(PageDirectoryEntry));
    kernelPDE.frame = (uint32_t)kernelPageTableAddress / BLOCK_SIZE;
    kernelPDE.pageSize = FOUR_KB;
    kernelPDE.isUser = 0;
    kernelPDE.readWrite = 1;
    kernelPDE.present = 1;

    //Dont asign it yet, first we fill in the page table
    PageTable* kernelPT = (PageTable*)phys2virt(kernelPageTableAddress); //We can do this since we have identity mapped the first 4mb, so it does not create a page fault
    //Identity map kernel's 4mb
    for(uint16_t i = 0; i < 1024; i++)
    {
        PageTableEntry pte;
        MemoryOperations::memset(&pte, 0, sizeof(pte));
        pte.frame = i;
        pte.isUser = 0;
        pte.readWrite = 1;
        pte.present = 1;

        //And then add it to the page table
        kernelPT->entries[i] = pte;
    }

    //Print the newly created PDE
    BootConsole::WriteLine("New Kernel Page Directory Entry");
    PrintPageDirectoryEntry(kernelPDE);

    //Then set the according entry in the page directory
    //Here we replace the old entry that was created by the loader
    pageDirectory->entries[KERNEL_PTNUM] = kernelPDE;

    //Finally set it as the current directory
    currentPageDirectory = pageDirectory;

    BootConsole::WriteLine("Reloading CR3 Register");
    ReloadCR3();
}


void* VirtualMemoryManager::VirtualToPhysical(void* virtualAddress, PageDirectory* dir)
{
    if(!dir)
        return 0;
    
    uint32_t pageDirectoryIndex = PAGEDIR_INDEX(virtualAddress);
    uint32_t pageTableIndex = PAGETBL_INDEX(virtualAddress);
    uint32_t pageFrameOffset = PAGEFRAME_INDEX(virtualAddress);

    if(dir->entries[pageDirectoryIndex].present == 0)
    {
        BootConsole::WriteLine("Page directory entry does not exist in page directory");
        return 0;
    }

    PageTable* pageTable = (PageTable*)(dir->entries[pageDirectoryIndex].frame * BLOCK_SIZE);
    if(pageTable == 0)
    {
        BootConsole::WriteLine("Page table does not exist at given index");
        return 0;
    }

    return (void*)(pageTable->entries[pageTableIndex].frame * BLOCK_SIZE | pageFrameOffset);
}

void VirtualMemoryManager::AllocatePage(PageTableEntry* page, bool kernel, bool writeable)
{
    void* p = PhysicalMemoryManager::AllocateBlock();
    if(!p)
        return;

    page->present = 1;
    page->readWrite = writeable ? 1 : 0;
    page->isUser = kernel ? 0 : 1;
    page->frame = (uint32_t)p / BLOCK_SIZE;
}

void VirtualMemoryManager::FreePage(PageTableEntry* page)
{
    void* addr = (void*)(page->frame * BLOCK_SIZE);
    if(addr)
        PhysicalMemoryManager::FreeBlock(addr);
    
    page->present = 0;
}