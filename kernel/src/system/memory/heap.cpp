#include <system/memory/heap.h>
#include <system/log.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint32_t KernelHeap::startAddress = 0;
uint32_t KernelHeap::endAddress = 0;
MemoryHeader* KernelHeap::firstHeader = 0;
bool KernelHeap::Enabled = false;

void KernelHeap::Initialize(uint32_t start, uint32_t end)
{
    if (start % PAGE_SIZE != 0 || end % PAGE_SIZE != 0)
    {
        BootConsole::WriteLine("Start or End is not page aligned");
        return;
    }

    KernelHeap::startAddress = start;
    KernelHeap::endAddress = end;

    //Heap should start after initrd, need to look into that in the future. But it works for now.
    //MemoryOperations::memset((void*)start, 0x0, end - start);

    firstHeader = (MemoryHeader*)start;
    firstHeader->allocated = false;
    firstHeader->prev = 0;
    firstHeader->next = 0;
    firstHeader->size = end - start - sizeof(MemoryHeader);
    
    #if USE_HEAP_MAGIC
    firstHeader->magic = MEMORY_HEADER_MAGIC;
    #endif

    KernelHeap::Enabled = true;
}

void* KernelHeap::InternalAllocate(uint32_t size)
{
    MemoryHeader* freeBlock = 0;

    for(MemoryHeader* hdr = firstHeader; hdr != 0 && freeBlock == 0; hdr = hdr->next)
        if(hdr->size > size && !hdr->allocated)
            freeBlock = hdr;
    
    if(freeBlock == 0)
    {
        Log(Error, "Out of Heap space!. This should never happen!");
        return 0;
    }

    if(freeBlock->size >= size + sizeof(MemoryHeader))
    {
        MemoryHeader* temp = (MemoryHeader*)((uint32_t)freeBlock + sizeof(MemoryHeader) + size);

        temp->allocated = false;
        temp->size = freeBlock->size - size - sizeof(MemoryHeader);
        temp->prev = freeBlock;
        temp->next = freeBlock->next;
        if(temp->next != 0)
            temp->next->prev = temp;

        freeBlock->size = size;
        freeBlock->next = temp;

        #if USE_HEAP_MAGIC
        temp->magic = MEMORY_HEADER_MAGIC;
        freeBlock->magic = MEMORY_HEADER_MAGIC;
        #endif
    }

    freeBlock->allocated = true;
    return (void*)(((uint32_t)freeBlock) + sizeof(MemoryHeader));
}
void KernelHeap::InternalFree(void* ptr)
{
    MemoryHeader* chunk = (MemoryHeader*)((uint32_t)ptr - sizeof(MemoryHeader));
    
    chunk -> allocated = false;
    
    if(chunk->prev != 0 && !chunk->prev->allocated)
    {
        chunk->prev->next = chunk->next;
        chunk->prev->size += chunk->size + sizeof(MemoryHeader);
        if(chunk->next != 0)
            chunk->next->prev = chunk->prev;
        
        chunk = chunk->prev;
    }
    
    if(chunk->next != 0 && !chunk->next->allocated)
    {
        chunk->size += chunk->next->size + sizeof(MemoryHeader);
        chunk->next = chunk->next->next;
        if(chunk->next != 0)
            chunk->next->prev = chunk;
    }   
}




void* KernelHeap::malloc(uint32_t size, uint32_t* physReturn)
{
    void* addr = InternalAllocate(size);
    if(physReturn != 0)
    {
        PageTableEntry* page = VirtualMemoryManager::GetPageForAddress((uint32_t)addr, 0);
        *physReturn = (page->frame * PAGE_SIZE) + ((uint32_t)addr & 0xFFF);
    }
    return addr;
}
void KernelHeap::free(void* ptr)
{
    InternalFree(ptr);
}

void* KernelHeap::allignedMalloc(uint32_t size, uint32_t align, uint32_t* physReturn)
{
    void* ptr = 0;

    if(!(align & (align - 1)) == 0)
        return 0;

    if(align && size)
    {
        uint32_t hdr_size = sizeof(uint16_t) + (align - 1);

        uint32_t phys = 0;
        void* p = malloc(size + hdr_size, &phys);

        if(p)
        {
            ptr = (void *) align_up(((uintptr_t)p + sizeof(uint16_t)), align);
            if(physReturn)
                *physReturn = (uint32_t)VirtualMemoryManager::virtualToPhysical(ptr); //align_up(phys, align);

            *((uint16_t*)ptr - 1) = (uint16_t)((uintptr_t)ptr - (uintptr_t)p);
        }
    }
    return ptr;
}
void KernelHeap::allignedFree(void* ptr)
{   
    if(ptr == 0)
        return;

    uint16_t offset = *((uint16_t *)ptr - 1);

    void * p = (void *)((common::uint8_t *)ptr - offset);

    free(p);
}

#if USE_HEAP_MAGIC
bool KernelHeap::CheckForErrors()
{
    for(MemoryHeader* hdr = firstHeader; hdr != 0; hdr = hdr->next)
        if(hdr->magic != MEMORY_HEADER_MAGIC)
            return true;
    
    return false;
}
#endif