#include <core/physicalmemory.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

uint32_t PhysicalMemoryManager::memorySize = 0;
uint32_t PhysicalMemoryManager::usedBlocks = 0;
uint32_t PhysicalMemoryManager::maximumBlocks = 0;
uint32_t *PhysicalMemoryManager::memoryArray = 0;

int PhysicalMemoryManager::FirstFree()
{
    for (uint32_t i = 0; i < TotalBlocks(); i++)
        if (memoryArray[i] != 0xffffffff)
            for (int j = 0; j < 32; j++)
            { //! test each bit in the dword

                int bit = 1 << j;
                if (!(memoryArray[i] & bit))
                    return i * 4 * 8 + j;
            }

    return -1;
}
int PhysicalMemoryManager::FirstFreeSize(uint32_t size)
{
    if (size == 0)
        return -1;

    if (size == 1)
        return FirstFree();

    for (uint32_t i = 0; i < TotalBlocks(); i++)
        if (memoryArray[i] != 0xffffffff)
            for (int j = 0; j < 32; j++)
            { //! test each bit in the dword

                int bit = 1 << j;
                if (!(memoryArray[i] & bit))
                {

                    int startingBit = i * 32;
                    startingBit += bit; //get the free bit in the dword at index i

                    uint32_t free = 0; //loop through each bit to see if its enough space
                    for (uint32_t count = 0; count <= size; count++)
                    {

                        if (!TestBit(startingBit + count))
                            free++; // this bit is clear (free frame)

                        if (free == size)
                            return i * 4 * 8 + j; //free count==size needed; return index
                    }
                }
            }

    return -1;
}

void PhysicalMemoryManager::Initialize(uint32_t size, uint32_t bitmap)
{
    BootConsole::Write("Memory Size: ");
    BootConsole::Write(Convert::IntToString(size / 1024 / 1024));
    BootConsole::WriteLine(" Mb");
    BootConsole::Write("Bitmap: 0x"); Print::printfHex32(bitmap); BootConsole::WriteLine();

    memorySize = size;
    memoryArray = (uint32_t *)bitmap;
    maximumBlocks = size / BLOCK_SIZE;
    usedBlocks = maximumBlocks; //We use all at startup

    MemoryOperations::memset(memoryArray, 0xf, usedBlocks / BLOCKS_PER_BYTE);

    BootConsole::Write("Bitmap size: ");
    BootConsole::Write(Convert::IntToString(GetBitmapSize() / 1024));
    BootConsole::WriteLine(" Kb");
}
void PhysicalMemoryManager::SetRegionFree(uint32_t base, uint32_t size)
{
    int align = base / BLOCK_SIZE;

    for (int blocks = size / BLOCK_SIZE; blocks > 0; blocks--)
    {
        UnsetBit(align++);
        usedBlocks--;
    }

    SetBit(0);
}
void PhysicalMemoryManager::SetRegionUsed(uint32_t base, uint32_t size)
{
    int align = base / BLOCK_SIZE;

    for (int blocks = size / BLOCK_SIZE; blocks > 0; blocks--)
    {
        SetBit(align++);
        usedBlocks++;
    }

    SetBit(0);
}
void PhysicalMemoryManager::ParseMemoryMap(const multiboot_info_t* mbi)
{
    BootConsole::WriteLine("Parsing grub memory map");
    BootConsole::Write("Address of mmap: 0x"); Print::printfHex32(phys2virt(mbi->mmap_addr)); BootConsole::WriteLine();

    grub_multiboot_memory_map_t *mmap = (grub_multiboot_memory_map_t *)phys2virt(mbi->mmap_addr);
    BootConsole::WriteLine("-------------------------------------------------");
    while ((unsigned int)mmap < phys2virt(mbi->mmap_addr) + mbi->mmap_length)
    {
        BootConsole::Write("|->   0x"); Print::printfHex32(mmap->base_addr_low); BootConsole::Write("   ");
        BootConsole::Write(Convert::IntToString(mmap->length_low / 1024)); BootConsole::Write(" Kb      ");
        BootConsole::SetX(35);
        BootConsole::Write("Type: "); BootConsole::Write(Convert::IntToString(mmap->type));

        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            BootConsole::Write(" [Free]");
            SetRegionFree(mmap->base_addr_low, mmap->length_low);
        }
        BootConsole::WriteLine();

        mmap = (grub_multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(unsigned int));
    }
    BootConsole::WriteLine("-------------------------------------------------");
}

void* PhysicalMemoryManager::AllocateBlock()
{
    if (FreeBlocks() <= 0)
        return 0;

    int frame = FirstFree();

    if (frame == -1)
        return 0;

    SetBit(frame);

    uint32_t addr = frame * BLOCK_SIZE;
    usedBlocks++;

    return (void *)addr;
}
void PhysicalMemoryManager::FreeBlock(void* ptr)
{
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / BLOCK_SIZE;

    UnsetBit(frame);

    usedBlocks--;
}
void* PhysicalMemoryManager::AllocateBlocks(uint32_t size)
{
    if (FreeBlocks() <= size)
        return 0; //not enough space

    int frame = FirstFreeSize(size);

    if (frame == -1)
        return 0; //not enough space

    for (uint32_t i = 0; i < size; i++)
        SetBit(frame + i);

    uint32_t addr = frame * BLOCK_SIZE;
    usedBlocks += size;

    return (void *)addr;
}
void PhysicalMemoryManager::FreeBlocks(void *ptr, uint32_t size)
{
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / BLOCK_SIZE;

    for (uint32_t i = 0; i < size; i++)
        UnsetBit(frame + i);

    usedBlocks -= size;
}

uint32_t PhysicalMemoryManager::AmountOfMemory()
{
    return memorySize;
}
uint32_t PhysicalMemoryManager::UsedBlocks()
{
    return usedBlocks;
}
uint32_t PhysicalMemoryManager::FreeBlocks()
{
    return maximumBlocks - usedBlocks;
}
uint32_t PhysicalMemoryManager::TotalBlocks()
{
    return maximumBlocks;
}
uint32_t PhysicalMemoryManager::GetBitmapSize()
{
    return memorySize / BLOCK_SIZE / BLOCKS_PER_BYTE;
}


/*//////////////
// Helper functions
*///////////////
uint32_t core::pageRoundUp(uint32_t address)
{
    if((address & 0xFFFFF000) != address)
    {
        address &= 0xFFFFF000;
        address += 0x1000;
    }
    return address;
}

uint32_t core::pageRoundDown(uint32_t address)
{
    return address & 0xFFFFF000;
}