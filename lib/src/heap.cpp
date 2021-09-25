#include <heap.h>
#include <syscall.h>
#include <log.h>

using namespace LIBCactusOS;

uint32_t UserHeap::startAddress = 0;
uint32_t UserHeap::endAddress = 0;
uint32_t UserHeap::maxAddress = 3_GB - 1_MB; //1 MB Below the kernel
MemoryHeader* UserHeap::firstHeader = 0;

uint32_t LIBCactusOS::pageRoundUp(uint32_t address)
{
    if((address & 0xFFFFF000) != address)
    {
        address &= 0xFFFFF000;
        address += 0x1000;
    }
    return address;
}
uint32_t LIBCactusOS::pageRoundDown(uint32_t address)
{
    return address & 0xFFFFF000;
}

void* UserHeap::Malloc(uint32_t size)
{
    MemoryHeader* freeBlock = 0;

    for(MemoryHeader* hdr = firstHeader; hdr != 0 && freeBlock == 0; hdr = hdr->next)
        if(hdr->size > size && !hdr->allocated)
            freeBlock = hdr;
    
    if(freeBlock == 0)
    {
        //We need to expand the heap
        Log(Warning, "Heap needs to be expanded for this process");
        
        //Call Kernel
        if(DoSyscall(SYSCALL_SET_HEAP_SIZE, pageRoundUp(endAddress + size + HEAP_INCREASE_SIZE)) == SYSCALL_RET_SUCCES) {
            MemoryHeader* lastHeader = 0;
            
            //Find last block
            for(MemoryHeader* hdr = firstHeader; hdr != 0 && freeBlock == 0; hdr = hdr->next)
                if(hdr->allocated == false)
                    lastHeader = hdr;
            
            lastHeader->size += size + HEAP_INCREASE_SIZE;

            endAddress = DoSyscall(SYSCALL_GET_HEAP_END);
            
            return Malloc(size);
        }
        else
            Log(Error, "Could not expand heap");
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
    }

    freeBlock->allocated = true;
    return (void*)(((uint32_t)freeBlock) + sizeof(MemoryHeader));
}
void UserHeap::Free(void* ptr)
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
void UserHeap::Initialize()
{
    startAddress = DoSyscall(SYSCALL_GET_HEAP_START);
    endAddress = DoSyscall(SYSCALL_GET_HEAP_END);

    firstHeader = (MemoryHeader*)startAddress;
    firstHeader->allocated = false;
    firstHeader->prev = 0;
    firstHeader->next = 0;
    firstHeader->size = endAddress - startAddress - sizeof(MemoryHeader);
}
void UserHeap::PrintMemoryLayout()
{
    int i = 0;
    for(MemoryHeader* hdr = firstHeader; hdr != 0; hdr = hdr->next)
    {
        Print("## (%d) ##### (%x) #########\n", i++, (uint32_t)hdr);
        Print("## Size: %d Allocated: %s\n", hdr->size, hdr->allocated ? "True" : "False");
    }
}
void* UserHeap::alignedMalloc(uint32_t size, uint32_t align)
{
    void* ptr = 0;

    if(!(align & (align - 1)) == 0)
        return 0;

    if(align && size)
    {
        uint32_t hdr_size = sizeof(uint16_t) + (align - 1);

        void* p = Malloc(size + hdr_size);

        if(p)
        {
            ptr = (void *) align_up(((uintptr_t)p + sizeof(uint16_t)), align);

            *((uint16_t*)ptr - 1) = (uint16_t)((uintptr_t)ptr - (uintptr_t)p);
        }
    }
    return ptr;
}
void UserHeap::allignedFree(void* ptr)
{
    if(ptr == 0)
        return;

    uint16_t offset = *((uint16_t*)ptr - 1);

    void* p = (void*)((uint8_t*)ptr - offset);

    Free(p);
}