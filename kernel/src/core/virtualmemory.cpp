#include <core/virtualmemory.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

static inline void invlpg(void* addr)
{
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

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
    
    //Re-use the page directory setup by the loader
    PageDirectory* pageDirectory = (PageDirectory*)&BootPageDirectory;

    //Set the last pde to the page directory itself
    //With this we can use recursive page tables
    PageDirectoryEntry lastPDE;
    MemoryOperations::memset(&lastPDE, 0, sizeof(PageDirectoryEntry));
    lastPDE.frame = virt2phys((uint32_t)&BootPageDirectory) / PAGE_SIZE;
    lastPDE.readWrite = 1;
    lastPDE.pageSize = FOUR_KB;
    lastPDE.present = 1;
    pageDirectory->entries[1023] = lastPDE;

    /*
    What we do here is create a new pagetable for the kernel, currently is mapped by a 4mb page by the bootloader
    But because we can (at least should) not access the physical allocated block directly, we need to add
    it to the page directory first. But when we do that the physical page table is empty or contains junk, here is where the problem occurs.
    Because of the junk qemu sometimes crashes and this could be posible on real hardware as well.
    So for now just use the 4mb page for the kernel setup by the loader until we find a solution to this.
    */
#if 0
    //One pagetable for the kernel
    void* kernelPageTablePhysAddress = PhysicalMemoryManager::AllocateBlock();
    PageDirectoryEntry kernelPDE;
    MemoryOperations::memset(&kernelPDE, 0, sizeof(PageDirectoryEntry));
    kernelPDE.frame = (uint32_t)kernelPageTablePhysAddress  / PAGE_SIZE;
    kernelPDE.readWrite = 1;
    kernelPDE.pageSize = FOUR_KB;
    kernelPDE.present = 1;
    pageDirectory->entries[KERNEL_PTNUM] = kernelPDE;

    //Fill in the page table
    PageTable* kernelPageTable = (PageTable*)GetPageTableAddress(KERNEL_PTNUM);
    for(uint16_t i = 0; i < 1024; i++)
    {
        kernelPageTable->entries[i].frame = i;
        kernelPageTable->entries[i].isUser = 0;
        kernelPageTable->entries[i].readWrite = 1;
        kernelPageTable->entries[i].present = 1;
    }
#endif
    //Here we map some pages for the intial kernel heap
    for(uint32_t i = KERNEL_HEAP_START; i < KERNEL_HEAP_START + KERNEL_HEAP_SIZE; i += PAGE_SIZE)
        AllocatePage(GetPageForAddress(i, true), true, true);
        

    //The first 4mb are identity mapped, this is needed for the smbios and the vm86 code
    //TODO: Make only the vm86 code accessible for userspace
    pageDirectory->entries[0].frame = 0;
    pageDirectory->entries[0].pageSize = FOUR_MB;
    pageDirectory->entries[0].isUser = 1; //VM86 code needs to run in first 1mb of memory and runs in level 3
    pageDirectory->entries[0].readWrite = 1;
    pageDirectory->entries[0].present = 1;

    //Finally reload the cr3 register
    ReloadCR3();
}

PageTableEntry* VirtualMemoryManager::GetPageForAddress(uint32_t virtualAddress, bool shouldCreate, bool readWrite, bool userPages)
{
    uint32_t pageDirIndex = PAGEDIR_INDEX(virtualAddress);
    uint32_t pageTableIndex = PAGETBL_INDEX(virtualAddress);

    PageDirectory* pageDir = (PageDirectory*)PAGE_DIRECTORY_ADDRESS;
    if(pageDir->entries[pageDirIndex].present == 0 && shouldCreate)
    {
        void* pageTablePhys = PhysicalMemoryManager::AllocateBlock();
        
        MemoryOperations::memset(&pageDir->entries[pageDirIndex], 0, sizeof(PageDirectoryEntry));
        pageDir->entries[pageDirIndex].frame = (uint32_t)pageTablePhys / PAGE_SIZE;
        pageDir->entries[pageDirIndex].readWrite = readWrite;
        pageDir->entries[pageDirIndex].isUser = userPages;
        pageDir->entries[pageDirIndex].pageSize = FOUR_KB;
        pageDir->entries[pageDirIndex].present = 1;

        PageTable* pageTableVirt = (PageTable*)GetPageTableAddress(pageDirIndex);
        MemoryOperations::memset(pageTableVirt, 0, sizeof(PageTable));

        return &(pageTableVirt->entries[pageTableIndex]);
    }

    PageTable* pageTable = (PageTable*)GetPageTableAddress(pageDirIndex);
    return &pageTable->entries[pageTableIndex];
}

void VirtualMemoryManager::AllocatePage(PageTableEntry* page, bool kernel, bool writeable)
{
    void* p = PhysicalMemoryManager::AllocateBlock();
    if(!p)
        return;

    page->present = 1;
    page->readWrite = writeable ? 1 : 0;
    page->isUser = kernel ? 0 : 1;
    page->frame = (uint32_t)p / PAGE_SIZE;
}

void VirtualMemoryManager::FreePage(PageTableEntry* page)
{
    void* addr = (void*)(page->frame * PAGE_SIZE);
    if(addr)
        PhysicalMemoryManager::FreeBlock(addr);
    
    page->present = 0;
}


void* VirtualMemoryManager::GetPageTableAddress(uint16_t pageTableNumber)
{
    uint32_t ret = PAGE_TABLE_ADDRESS;
    ret |= (pageTableNumber << PAGE_OFFSET_BITS);
    return (void*)ret;
}

void* VirtualMemoryManager::virtualToPhysical(void* virtAddress)
{
    uint32_t pd_offset = PAGEDIR_INDEX(virtAddress);
    uint32_t pt_offset = PAGETBL_INDEX(virtAddress);
    uint32_t p_offset = PAGEFRAME_INDEX(virtAddress);

    PageTable* pageTable = (PageTable*)(PAGE_TABLE_ADDRESS + (PAGE_SIZE * pd_offset));
    PageTableEntry pageTableEntry = pageTable->entries[pt_offset];

    uint32_t physAddress = (pageTableEntry.frame * PAGE_SIZE) | p_offset;

    return (void*)physAddress;
}

void VirtualMemoryManager::mapVirtualToPhysical(void* physAddress, void* virtAddress, bool kernel, bool writeable)
{
    PageTableEntry* page = GetPageForAddress((uint32_t)virtAddress, true, writeable, !kernel);

    page->frame = (uint32_t)physAddress / PAGE_SIZE;
    page->isUser = kernel ? 0 : 1;
    page->readWrite = writeable ? 1 : 0;
    page->present = 1;

    invlpg(virtAddress);
}

void VirtualMemoryManager::mapVirtualToPhysical(void* physAddress, void* virtAddress, uint32_t size, bool kernel, bool writeable)
{
    if(size % PAGE_SIZE != 0)
        return;
    
    for(uint32_t i = 0; i < size; i += PAGE_SIZE)
    {
        mapVirtualToPhysical((void*)((uint32_t)physAddress + i), (void*)((uint32_t)virtAddress + i), kernel, writeable);
    }
}

void VirtualMemoryManager::SwitchPageDirectory(uint32_t physAddr)
{
    asm volatile("mov %0, %%cr3" :: "r"(physAddr));
}

uint32_t VirtualMemoryManager::GetPageDirectoryAddress()
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}