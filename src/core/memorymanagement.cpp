#include <core/memorymanagement.h>

using namespace CactusOS;
using namespace CactusOS::core;

void printf(char*);

MemoryManager* MemoryManager::activeMemoryManager = 0;

int TotalMemory = 0;
int UsedMemory = 0;
        
MemoryManager::MemoryManager(common::size_t start, common::size_t size)
{
    activeMemoryManager = this;
    TotalMemory = size;
    UsedMemory = 0;
    
    if(size < sizeof(MemoryChunk))
    {
        first = 0;
    }
    else
    {
        first = (MemoryChunk*)start;
        
        first -> allocated = false;
        first -> prev = 0;
        first -> next = 0;
        first -> size = size - sizeof(MemoryChunk);
    }
}

MemoryManager::~MemoryManager()
{
    if(activeMemoryManager == this)
        activeMemoryManager = 0;
}
        
void* MemoryManager::malloc(common::size_t size)
{
    MemoryChunk *result = 0;
    
    for(MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next)
        if(chunk->size > size && !chunk->allocated)
            result = chunk;
        
    if(result == 0)
    {
        printf("Error could not allocate memory! amount: "); printf(common::Convert::IntToString(size)); printf(" Bytes\n");
        printf("Memory layout:\n");

        for(MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next)
        {
            printf("[Size: "); printf(common::Convert::IntToString(chunk->size)); printf(" Alloc: "); printf(chunk->allocated ? (char*)"True" : (char*)"False"); printf("]\n");
        }

        while(1);
        return 0;
    }
    
    if(result->size >= size + sizeof(MemoryChunk) + 1)
    {
        MemoryChunk* temp = (MemoryChunk*)((common::size_t)result + sizeof(MemoryChunk) + size);
        
        temp->allocated = false;
        temp->size = result->size - size - sizeof(MemoryChunk);
        temp->prev = result;
        temp->next = result->next;
        if(temp->next != 0)
            temp->next->prev = temp;
        
        result->size = size;
        result->next = temp;
    }
    
    result->allocated = true;
    UsedMemory += size;

    return (void*)(((common::size_t)result) + sizeof(MemoryChunk));
}

void MemoryManager::free(void* ptr)
{
    MemoryChunk* chunk = (MemoryChunk*)((common::size_t)ptr - sizeof(MemoryChunk));
    
    chunk -> allocated = false;
    UsedMemory -= chunk->size;
    
    if(chunk->prev != 0 && !chunk->prev->allocated)
    {
        chunk->prev->next = chunk->next;
        chunk->prev->size += chunk->size + sizeof(MemoryChunk);
        if(chunk->next != 0)
            chunk->next->prev = chunk->prev;
        
        chunk = chunk->prev;
    }
    
    if(chunk->next != 0 && !chunk->next->allocated)
    {
        chunk->size += chunk->next->size + sizeof(MemoryChunk);
        chunk->next = chunk->next->next;
        if(chunk->next != 0)
            chunk->next->prev = chunk;
    }
}

void* MemoryManager::aligned_malloc(common::size_t align, common::size_t size)
{
    void * ptr = NULL;

    //We want it to be a power of two since align_up operates on powers of two
    if(!(align & (align - 1)) == 0)
        return 0;

    if(align && size)
    {
        /*
         * We know we have to fit an offset value
         * We also allocate extra bytes to ensure we can meet the alignment
         */
        common::uint32_t hdr_size = sizeof(offset_t) + (align - 1);
        void * p = malloc(size + hdr_size);

        if(p)
        {
            /*
             * Add the offset size to malloc's pointer (we will always store that)
             * Then align the resulting value to the arget alignment
             */
            ptr = (void *) align_up(((common::uintptr_t)p + sizeof(offset_t)), align);

            //Calculate the offset and store it behind our aligned pointer
            *((offset_t *)ptr - 1) = (offset_t)((common::uintptr_t)ptr - (common::uintptr_t)p);

        } // else NULL, could not malloc
    } //else NULL, invalid arguments

    return ptr;
}
void MemoryManager::aligned_free(void* ptr)
{
    if(ptr == 0)
        return;

    /*
    * Walk backwards from the passed-in pointer to get the pointer offset
    * We convert to an offset_t pointer and rely on pointer math to get the data
    */
    offset_t offset = *((offset_t *)ptr - 1);

    /*
    * Once we have the offset, we can get our original pointer and call free
    */
    void * p = (void *)((common::uint8_t *)ptr - offset);
    free(p);
}

common::uint32_t MemoryManager::GetTotalMemory()
{
    return TotalMemory;
}
common::uint32_t MemoryManager::GetUsedMemory()
{
    return UsedMemory;
}
common::uint32_t MemoryManager::GetFreeMemory()
{
    return TotalMemory - UsedMemory;
}




void* operator new(size_t size)
{
    if(MemoryManager::activeMemoryManager == 0)
        return (void*)0;
    return MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new[](size_t size)
{
    if(MemoryManager::activeMemoryManager == 0)
        return (void*)0;
    return MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new(size_t size, void* ptr)
{
    return ptr;
}

void* operator new[](size_t size, void* ptr)
{
    return ptr;
}

void operator delete(void* ptr)
{
    if(MemoryManager::activeMemoryManager != 0)
        MemoryManager::activeMemoryManager->free(ptr);
}

void operator delete[](void* ptr)
{
    if(MemoryManager::activeMemoryManager != 0)
        MemoryManager::activeMemoryManager->free(ptr);
}
void operator delete(void* ptr, unsigned long size)
{
    if(MemoryManager::activeMemoryManager != 0)
        MemoryManager::activeMemoryManager->free(ptr);
}
void operator delete[](void* ptr, unsigned long size)
{
    if(MemoryManager::activeMemoryManager != 0)
        MemoryManager::activeMemoryManager->free(ptr);
}