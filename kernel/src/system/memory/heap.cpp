#include <system/memory/heap.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint32_t KernelHeap::startAddress = 0;
uint32_t KernelHeap::endAddress = 0;
MemoryHeader* KernelHeap::firstHeader = 0;
MutexLock KernelHeap::heapMutex = MutexLock();

void KernelHeap::Initialize(uint32_t start, uint32_t end)
{
    Log(Info, "KernelHeap: Initializing (Size of MemoryHeader = %d)", sizeof(MemoryHeader));
    if (start % PAGE_SIZE != 0 || end % PAGE_SIZE != 0) {
        Log(Error, "KernelHeap: Start or End is not page aligned");
        System::Panic();
    }

    // Update static variables
    KernelHeap::startAddress = start;
    KernelHeap::endAddress = end;

    // Initialize first memoryheader with right variables
    firstHeader = (MemoryHeader*)start;
    firstHeader->allocated = false;
    firstHeader->prev = 0;
    firstHeader->next = 0;
    firstHeader->size = end - start - sizeof(MemoryHeader); // Make this the size of the whole memory range   
    firstHeader->magic = MEMORY_HEADER_MAGIC;
}

MemoryHeader* KernelHeap::FirstFree(uint32_t size)
{
    MemoryHeader* block = firstHeader;
    while(block != 0 && block->magic == MEMORY_HEADER_MAGIC) {
        if(block->allocated || block->size < size) {
            block = block->next;
            continue; // Continue search
        }

        return block; // This block seems fine
    }
    return 0;
}

void* KernelHeap::InternalAllocate(uint32_t size)
{
    // Set Mutex
    heapMutex.Lock();

    // First we align the size to a 4-byte boundary
    // This makes accessing memory a bit faster
    size = align_up(size, sizeof(uint32_t));

    // Search for free block
    MemoryHeader* freeBlock = FirstFree(size);
    
    // Check for error
    if(freeBlock == 0) {
        Log(Error, "KernelHeap: Out of Heap space!. This should never happen!");
        System::Panic();
        heapMutex.Unlock(); // Not really required but perhaps panic function will return in the future
        return 0;
    }

    // Check if we have space for a extra memory block (and it is worth it)
    if(freeBlock->size >= (size + sizeof(MemoryHeader) + MINIMAL_SPLIT_SIZE))
    {
        // Split current found block into 2 seperate ones
        MemoryHeader* nextBlock = (MemoryHeader*)((uint32_t)freeBlock + size + sizeof(MemoryHeader));

        // Setup new block
        nextBlock->allocated = false;
        nextBlock->size = freeBlock->size - size - sizeof(MemoryHeader);
        nextBlock->prev = freeBlock;
        nextBlock->next = freeBlock->next;
        if(freeBlock->next != 0)
            freeBlock->next->prev = nextBlock;

        // Setup magic number for new block
        nextBlock->magic = MEMORY_HEADER_MAGIC;

        // And point new block to next memory block
        freeBlock->next = nextBlock;
    }

    freeBlock->allocated = true;
    freeBlock->size = size;
    
    // Free mutex
    heapMutex.Unlock();

    return (void*)((uint32_t)freeBlock + sizeof(MemoryHeader));
}
void KernelHeap::free(void* ptr)
{
    // Set mutex
    heapMutex.Lock();

    // Get pointer to memory block associated with pointer
    MemoryHeader* block = (MemoryHeader*)((uint32_t)ptr - sizeof(MemoryHeader));

    // Check if block is valid
    if(block->magic != MEMORY_HEADER_MAGIC || !block->allocated) {
        Log(Error, "KernelHeap: Block is not allocated or magic is not correct");
        Log(Error, "Block = %x Alloc = %d Size = %d", (uint32_t)block, block->allocated, block->size);
        System::Panic();
    }
    
    // Block is not allocated anymore
    block->allocated = false;

    // Check if we can merge this block with the previous one
    if(block->prev && !block->prev->allocated)
    {
        // Make previous block skip this block and point to the next one
        block->prev->next = block->next;

        // Increase size with this blocks size
        block->prev->size += block->size + sizeof(MemoryHeader);
        
        // Update prev pointer for next block
        if(block->next != 0)
            block->next->prev = block->prev;
        
        // We are now the previous block
        block = block->prev;
    }
    
    // Check if we can merge this block with the next one
    if(block->next && !block->next->allocated)
    {
        // Increase this blocks size
        block->size += block->next->size + sizeof(MemoryHeader);

        // Update next pointer
        block->next = block->next->next;

        // Update prev pointer for next block
        if(block->next != 0)
            block->next->prev = block;
    }

    // Unlock mutex
    heapMutex.Unlock();
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

void* KernelHeap::alignedMalloc(uint32_t size, uint32_t align, uint32_t* physReturn)
{
    void* ptr = 0;
    if(!(align & (align - 1)) == 0)
        return 0;

    if(align > 0 && size > 0)
    {
        uint32_t hdr_size = sizeof(uint16_t) + (align - 1);
        uint32_t phys = 0;
        void* block = malloc(size + hdr_size, &phys);
        if(block)
        {
            ptr = (void*)align_up(((uintptr_t)block + sizeof(uint16_t)), align);
            if(physReturn)
                *physReturn = (uint32_t)VirtualMemoryManager::virtualToPhysical(ptr);

            *((uint16_t*)ptr - 1) = (uint16_t)((uintptr_t)ptr - (uintptr_t)block);
        }
    }

    return ptr;
}
void KernelHeap::allignedFree(void* ptr)
{   
    if(ptr == 0)
        return;

    uint16_t offset = *((uint16_t*)ptr - 1);
    void* newPtr = (void*)((common::uint8_t*)ptr - offset);
    free(newPtr);
}

bool KernelHeap::CheckForErrors()
{
    // Not sure if this is needed, but why not?
    heapMutex.Lock();

    MemoryHeader* block = firstHeader;
    while(block != 0) {
        if(((block->magic != MEMORY_HEADER_MAGIC) || (((uint32_t)block->next < startAddress) || ((uint32_t)block->next > endAddress))) && (block->next != 0)) {
            heapMutex.Unlock();
            
            Log(Error, "Memory corrupted at: %x (magic = %d, next = %x, alloc = %d)", (uint32_t)block, block->magic, (uint32_t)block->next, block->allocated);
            return true;
        }
        block = block->next;
    }
    heapMutex.Unlock();
    return false;
}

uint32_t KernelHeap::UsedMemory()
{
    uint32_t result = 0;

    MemoryHeader* block = firstHeader;
    while(block != 0) {
        if(((block->magic != MEMORY_HEADER_MAGIC) || (((uint32_t)block->next < startAddress) || ((uint32_t)block->next > endAddress))) && (block->next != 0)) {
            Log(Error, "Memory corrupted at: %x (magic = %d, next = %x, alloc = %d)", (uint32_t)block, block->magic, (uint32_t)block->next, block->allocated);
            return result;
        }
        if(block->allocated)
            result += block->size;
        
        block = block->next;
    }

    return result;
}