#include <system/tasking/process.h>

#include <system/system.h>
#include <system/memory/deviceheap.h>
#include <system/tasking/elf.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

static int currentPID = 1;
List<Process*> ProcessHelper::Processes;

ProcessHelper::ProcessHelper()
{   }

Process* ProcessHelper::Create(char* fileName, char* arguments, bool isKernel)
{
    // Check if the file exists
    if(!System::vfs->FileExists(fileName))
        return 0;

    // Get the filesize
    int fileSize = System::vfs->GetFileSize(fileName);
    if(fileSize == -1)
        return 0;

    // Allocate a buffer to read the bin
    uint8_t* fileBuffer = new uint8_t[fileSize];

    if(System::vfs->ReadFile(fileName, fileBuffer) != 0) //An error occurred
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
    PageDirectory* pageDir = (PageDirectory*)KernelHeap::alignedMalloc(sizeof(PageDirectory), sizeof(PageDirectory), &pageDirPhys);
    MemoryOperations::memset(pageDir, 0, sizeof(PageDirectory));

    // Copy kernel pages
    pageDir->entries[KERNEL_PTNUM] = ((PageDirectory*)&BootPageDirectory)->entries[KERNEL_PTNUM];
    
    // Copy kernel heap as well
    for(uint32_t i = 0; i < KERNEL_HEAP_SIZE / 4_MB; i++)
        pageDir->entries[KERNEL_PTNUM + i + 1] = ((PageDirectory*)&BootPageDirectory)->entries[KERNEL_PTNUM + i + 1];

    // We also need to copy memory used by devices to this process
    // We assume all memory is initialized when the first process is started
    for(uint32_t i = DEVICE_HEAP_START; i < (DEVICE_HEAP_START + DEVICE_HEAP_SIZE); i += 4_MB)
        pageDir->entries[PAGEDIR_INDEX(i)] = ((PageDirectory*)&BootPageDirectory)->entries[PAGEDIR_INDEX(i)];

    // Set the last pde to the page directory itself
    // With this we can use recursive page tables
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
            // Should the pages for section be read only or write as well?
            // This way we can prevent that usercode modifies itself
            bool rw = prgmHeader->p_flags & (1<<1);

            // Allocate pages for section
            for(uint32_t x = 0; x < prgmHeader->p_memsz; x+=PAGE_SIZE)
                VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(prgmHeader->p_vaddr + x, true, true, !isKernel), isKernel, rw);

            // Store memory information about excecutable
			if (prgmHeader->p_vaddr < proc->excecutable.memBase || proc->excecutable.memBase == 0) {
				proc->excecutable.memBase = prgmHeader->p_vaddr;
			}
			if (prgmHeader->p_vaddr + prgmHeader->p_memsz - proc->excecutable.memBase > proc->excecutable.memSize) {
				proc->excecutable.memSize = prgmHeader->p_vaddr + prgmHeader->p_memsz - proc->excecutable.memBase;
            }
        }
    
    VirtualMemoryManager::ReloadCR3();

    // Reset it otherwise the code will not be copied
    prgmHeader = (ElfProgramHeader*)(fileBuffer + header->e_phoff);

    // Load application by copying all loadable sections into memory
    for(int i = 0; i < header->e_phnum; i++, prgmHeader++) {
        //Log(Info, "[Process.Create] Program header type %d", prgmHeader->p_type);
        if(prgmHeader->p_type == 1) {
            //Log(Info, "[Process.Create] p_memsz = %d, p_filesz = %d", prgmHeader->p_memsz, prgmHeader->p_filesz);
            
            // First copy part that is actually present in file
            MemoryOperations::memcpy((void*)prgmHeader->p_vaddr, fileBuffer + prgmHeader->p_offset, prgmHeader->p_filesz);

            // Calculate remaning space (if there is some)
            uint32_t rem = prgmHeader->p_memsz - prgmHeader->p_filesz;
            uint32_t off = prgmHeader->p_filesz;

            // And clear remaning memory if necessary
            if(rem) MemoryOperations::memset((void*)(prgmHeader->p_vaddr + off), 0, rem);
        }
    }

    // Put information in PCB
    proc->id = currentPID++;
    proc->pageDirPhys = pageDirPhys;
    proc->state = ProcessState::Active;
    proc->isUserspace = !isKernel;
    proc->args = arguments;
    proc->Threads.push_back(ThreadHelper::CreateFromFunction((void (*)())header->e_entry, isKernel));

    Thread* mainThread = proc->Threads[0];

    // Create userstack for process
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

    //Create stream for input
    proc->stdInput = new FIFOStream(10_KB);
    //Redirect output to system console
    proc->stdOutput = System::ProcStandardOut;
   
    mainThread->parent = proc;

    delete fileBuffer;

    VirtualMemoryManager::SwitchPageDirectory(oldCR3);

    //We can only copy the filename when we are in the same virtual space as the application that requested the exec
    int fileNameLen = String::strlen(fileName);
    MemoryOperations::memcpy(proc->fileName, fileName, fileNameLen <= 32 ? fileNameLen : 32);

    InterruptDescriptorTable::EnableInterrupts();

    // Assign debugger if demanded
#if ENABLE_ADV_DEBUG
    char* symbolFile = new char[fileNameLen+1];
    MemoryOperations::memset(symbolFile, 0, fileNameLen);
    MemoryOperations::memcpy(symbolFile, fileName, fileNameLen - 3);
    MemoryOperations::memcpy(symbolFile + fileNameLen - 3, "sym", 4);
    proc->symDebugger = new SymbolDebugger(symbolFile);
    delete symbolFile;
#endif

    Processes.push_back(proc); //Finally add it to all known processes

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
    proc->heap.heapStart = KERNEL_HEAP_START;
    proc->heap.heapEnd = KERNEL_HEAP_SIZE;
    
    //Finally add it to all known processes
    Processes.push_back(proc);

    return proc;
}

void ProcessHelper::RemoveProcess(Process* proc)
{
    Log(Info, "Removing process %d from system", proc->id);
    InterruptDescriptorTable::DisableInterrupts(); //We do not want to be interrupted during the switch
    Processes.Remove(proc); //Remove the process from the list

    for(int i = 0; i < proc->Threads.size(); i++)
        ThreadHelper::RemoveThread(proc->Threads[i]);

    //Free pages used by excecutable code
    for(uint32_t p = proc->excecutable.memBase; p < proc->excecutable.memBase + proc->excecutable.memSize; p+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(p, false));

    //Free pages used by the heap
    for(uint32_t p = proc->heap.heapStart; p < proc->heap.heapEnd; p+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(p, false));

    //Delete ipc messages
    proc->ipcMessages.Clear();
    
    //Remove processes output that point to this process input
    for(int i = 0; i < Processes.size(); i++)
        if(Processes[i]->stdOutput == proc->stdInput)
            Processes[i]->stdOutput = 0;
    
    //Free memory used by stdinput if possible
    if(proc->stdInput != 0 && proc->stdInput != System::keyboardManager)
        delete proc->stdInput;

    delete proc;

    InterruptDescriptorTable::EnableInterrupts();

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
    else if(proc->heap.heapEnd > newEndAddr) //shrink
    {
        Log(Info, "shrinking heap (PID: %d) from %x to %x", proc->id, proc->heap.heapEnd, newEndAddr);

        for(uint32_t i = pageRoundUp(newEndAddr); i < proc->heap.heapEnd; i+=PAGE_SIZE)
            VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(i, false, true, proc->isUserspace));
        
        proc->heap.heapEnd = pageRoundUp(newEndAddr);
    }
}

Process* ProcessHelper::ProcessById(int id)
{
    for(Process* p : Processes)
        if(p->id == id)
            return p;
            
    return 0;
}