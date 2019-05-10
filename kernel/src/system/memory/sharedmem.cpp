#include <system/memory/sharedmem.h>
#include <core/virtualmemory.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

bool SharedMemory::CreateSharedRegion(Process* proc1, Process* proc2, uint32_t virtStart, uint32_t len)
{
    return SharedMemory::CreateSharedRegion(proc1, proc2, virtStart, virtStart, len);
}
bool SharedMemory::CreateSharedRegion(Process* proc1, Process* proc2, uint32_t virtStart1, uint32_t virtStart2, uint32_t len)
{
    if(proc1 == 0 || proc2 == 0 || len <= 0)
        return false;

    //Since we are messing with pagetables we need to disable the scheduler
    System::scheduler->Enabled = false;

    //Allocate the required memory block
    uint32_t physMemStart = (uint32_t)PhysicalMemoryManager::AllocateBlocks(len / PAGE_SIZE);
    Log(Info, "Allocated shared memory block of %d bytes at %x", len, physMemStart);

    uint32_t oldCR3 = VirtualMemoryManager::GetPageDirectoryAddress();
    //Check if we are running proc1 already, so we don't have to switch the pagedir
    if(oldCR3 != proc1->pageDirPhys) {
        VirtualMemoryManager::SwitchPageDirectory(proc1->pageDirPhys);
    }
    
    //Map the memory region for process 1
    for(uint32_t memAddr = 0; memAddr < len; memAddr += PAGE_SIZE)
    {
        //Get or create page for address
        PageTableEntry* page = VirtualMemoryManager::GetPageForAddress(memAddr + virtStart1, true, true, true);

        page->frame = (physMemStart + memAddr) / PAGE_SIZE; //Assign the correct memory address
        page->readWrite = 1;
        page->isUser = 1;
        page->present = 1;
    }

    //Switch to process 2 pagedir
    VirtualMemoryManager::SwitchPageDirectory(proc2->pageDirPhys);

    //Map the memory region for process 2
    for(uint32_t memAddr = 0; memAddr < len; memAddr += PAGE_SIZE)
    {
        //Get or create page for address
        PageTableEntry* page = VirtualMemoryManager::GetPageForAddress(memAddr + virtStart2, true, true, true);

        page->frame = (physMemStart + memAddr) / PAGE_SIZE; //Assign the correct memory address
        page->readWrite = 1;
        page->isUser = 1;
        page->present = 1;
    }

    //Restore page directory
    VirtualMemoryManager::SwitchPageDirectory(oldCR3);

    System::scheduler->Enabled = true;
    
    return true;
}