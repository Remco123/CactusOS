#include <system/tasking/process.h>

#include <system/system.h>
#include <system/tasking/elf.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

static int currentPID = 1;

ProcessHelper::ProcessHelper()
{   }

Process* ProcessHelper::Create(char* fileName, bool isKernel)
{
    //Check if the file exists
    if(!System::vfs->FileExists(fileName))
        return 0;

    //Get the filesize
    int fileSize = System::vfs->GetFileSize(fileName);
    if(fileSize == -1)
        return 0;

    //Allocate a buffer to read the bin
    uint8_t* fileBuffer = new uint8_t[fileSize];

    if(System::vfs->ReadFile(fileName, fileBuffer) != 0) //An error occured
    {
        delete fileBuffer;
        return 0;
    }

    InterruptDescriptorTable::DisableInterrupts();

    /*////////////////////
    Check for valid elf file
    */////////////////////
    ElfHeader* header = (ElfHeader*)fileBuffer;

    if(!(header->e_ident[0] == ELFMAG0 && header->e_ident[1] == ELFMAG1 && header->e_ident[2] == ELFMAG2 && header->e_ident[3] == ELFMAG3) || header->e_type != 2)
    {
        delete fileBuffer;
        return 0;
    }

    /*////////////////////
    Create addres space
    */////////////////////
    uint32_t pageDirPhys = 0;
    PageDirectory* pageDir = (PageDirectory*)KernelHeap::allignedMalloc(sizeof(PageDirectory), sizeof(PageDirectory), &pageDirPhys);
    MemoryOperations::memset(pageDir, 0, sizeof(PageDirectory));

    //Copy kernel pages
    pageDir->entries[KERNEL_PTNUM] = ((PageDirectory*)&BootPageDirectory)->entries[KERNEL_PTNUM];
    pageDir->entries[KERNEL_PTNUM + 1] = ((PageDirectory*)&BootPageDirectory)->entries[KERNEL_PTNUM + 1];

    //Set the last pde to the page directory itself
    //With this we can use recursive page tables
    PageDirectoryEntry lastPDE;
    MemoryOperations::memset(&lastPDE, 0, sizeof(PageDirectoryEntry));
    lastPDE.frame = pageDirPhys / PAGE_SIZE;
    lastPDE.readWrite = 1;
    lastPDE.pageSize = FOUR_KB;
    lastPDE.present = 1;
    pageDir->entries[1023] = lastPDE;

    uint32_t oldCR3 = VirtualMemoryManager::GetPageDirectoryAddress();
    VirtualMemoryManager::SwitchPageDirectory(pageDirPhys);

    /*////////////////////
    Create PCB
    */////////////////////
    Process* proc = new Process();
    MemoryOperations::memset(proc, 0, sizeof(Process));

    /*////////////////////
    Load process into memory
    */////////////////////
    ElfProgramHeader* prgmHeader = (ElfProgramHeader*)(fileBuffer + header->e_phoff);

    for(int i = 0; i < header->e_phnum; i++, prgmHeader++)
        if(prgmHeader->p_type == 1)
        {
            //Allocate pages for section
            for(uint32_t x = 0; x < prgmHeader->p_memsz; x+=PAGE_SIZE)
                VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(prgmHeader->p_vaddr + x, true, true, !isKernel), isKernel, true);

            //Store memory information about excecutable
			if (prgmHeader->p_vaddr < proc->excecutable.memBase || proc->excecutable.memBase == 0) {
				proc->excecutable.memBase = prgmHeader->p_vaddr;
			}
			if (prgmHeader->p_vaddr + prgmHeader->p_memsz - proc->excecutable.memBase > proc->excecutable.memSize) {
				proc->excecutable.memSize = prgmHeader->p_vaddr + prgmHeader->p_memsz - proc->excecutable.memBase;
            }
        }
    
    VirtualMemoryManager::ReloadCR3();

    //Reset it otherwise the code will not be copied
    prgmHeader = (ElfProgramHeader*)(fileBuffer + header->e_phoff);

    for(int i = 0; i < header->e_phnum; i++, prgmHeader++)
        if(prgmHeader->p_type == 1) 
            MemoryOperations::memcpy((void*)prgmHeader->p_vaddr, fileBuffer + prgmHeader->p_offset, prgmHeader->p_memsz);

    //Put information in PCB
    MemoryOperations::memcpy(proc->fileName, fileName, String::strlen(fileName));
    proc->id = currentPID++;
    proc->pageDirPhys = pageDirPhys;
    proc->state = ProcessState::Active;
    proc->isUserspace = !isKernel;
    proc->Threads.push_back(ThreadHelper::CreateFromFunction((void (*)())header->e_entry, isKernel));

    Thread* mainThread = proc->Threads[0];

    //Create userstack for process
    for(uint32_t i = (uint32_t)mainThread->userStack; i < ((uint32_t)mainThread->userStack + mainThread->userStackSize); i+=PAGE_SIZE)
        VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(i, true, true, !isKernel), isKernel, true);

    //Create heap for user process
    for(uint32_t i = 0; i < PROC_USER_HEAP_SIZE; i+=PAGE_SIZE)
    {
        uint32_t addr = pageRoundUp(proc->excecutable.memBase + proc->excecutable.memSize) + i;
        VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(addr, true, true, !isKernel), isKernel, true);
    }

    proc->heap.heapStart = pageRoundUp(proc->excecutable.memBase + proc->excecutable.memSize);
    proc->heap.heapEnd = proc->heap.heapStart + PROC_USER_HEAP_SIZE;
   
    mainThread->parent = proc;

    delete fileBuffer;

    VirtualMemoryManager::SwitchPageDirectory(oldCR3);

    InterruptDescriptorTable::EnableInterrupts();

    return proc;
}   

Process* ProcessHelper::CreateKernelProcess()
{
    /*////////////////////
    Create PCB
    */////////////////////
    Process* proc = new Process();
    MemoryOperations::memset(proc, 0, sizeof(Process));

    MemoryOperations::memcpy(proc->fileName, "Kernel Process", 15);
    proc->id = currentPID++;
    proc->pageDirPhys = virt2phys((uint32_t)&BootPageDirectory);
    proc->state = ProcessState::Active;
    proc->syscallID = 1;

    return proc;
}

void ProcessHelper::RemoveProcess(Process* proc)
{
    Log(Info, "Removing process %d from system", proc->id);
    System::scheduler->Enabled = false; //We do not want to be interrupted during the switch

    for(int i = 0; i < proc->Threads.size(); i++)
        ThreadHelper::RemoveThread(proc->Threads[i]);

    //Free pages used by excecutable code
    for(uint32_t p = proc->excecutable.memBase; p < proc->excecutable.memBase + proc->excecutable.memSize; p+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(p, false));

    //Free pages used by the heap
    for(uint32_t p = proc->heap.heapStart; p < proc->heap.heapEnd; p+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(p, false));

    delete proc;

    //Finally force a contex switch so that we never return to this process again.
    System::scheduler->ForceSwitch();
}

void ProcessHelper::UpdateHeap(Process* proc, uint32_t newEndAddr)
{
    if(proc->heap.heapEnd < newEndAddr) //Expand
    {
        Log(Info, "Expanding heap (PID: %d) from %x to %x", proc->id, proc->heap.heapEnd, newEndAddr);
        
        for(uint32_t i = proc->heap.heapEnd; i < pageRoundUp(newEndAddr); i+=PAGE_SIZE)
            VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(i, true, true, proc->isUserspace), !proc->isUserspace, true);
        
        proc->heap.heapEnd = pageRoundUp(newEndAddr);
    }
    else if(proc->heap.heapEnd > newEndAddr) //Contract
    {

    }
}