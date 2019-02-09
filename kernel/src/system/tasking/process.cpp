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
    Load process into memory
    */////////////////////
    ElfProgramHeader* prgmHeader = (ElfProgramHeader*)(fileBuffer + header->e_phoff);

    for(int i = 0; i < header->e_phnum; i++, prgmHeader++)
    {
        if(prgmHeader->p_type == 1)
        {   
            for(uint32_t x = 0; x < prgmHeader->p_memsz; x+=PAGE_SIZE)
                VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(prgmHeader->p_vaddr + x, true, true, !isKernel), isKernel, true);

            VirtualMemoryManager::ReloadCR3();

            MemoryOperations::memcpy((void*)prgmHeader->p_vaddr, fileBuffer + prgmHeader->p_offset, prgmHeader->p_memsz);
        }
    }

    /*////////////////////
    Create PCB
    */////////////////////
    Process* proc = new Process();
    MemoryOperations::memset(proc, 0, sizeof(Process));

    MemoryOperations::memcpy(proc->fileName, fileName, String::strlen(fileName));
    proc->id = currentPID++;
    proc->pageDirPhys = pageDirPhys;
    proc->state = ProcessState::Active;
    proc->Threads.push_back(ThreadHelper::CreateFromFunction((void (*)())header->e_entry, isKernel));

    proc->Threads[0]->parent = proc;

    VirtualMemoryManager::SwitchPageDirectory(oldCR3);

    InterruptDescriptorTable::EnableInterrupts();

    return proc;
}   