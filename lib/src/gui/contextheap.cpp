#include <gui/contextheap.h>
#include <list.h>
#include <string.h>
#include <proc.h>

using namespace LIBCactusOS;

#define ENTRIES_PER_BYTE 8
#define ARRAY_ENTRY_SIZE 4
#define ARRAY_ENTRIES (blockCount / ENTRIES_PER_BYTE / ARRAY_ENTRY_SIZE)

const int blockSize = 0x1000;
const uint32_t startAddress = 0xA0000000;
const uint32_t endAddress = 0xAB000000;

uint32_t* memoryBitmap = 0;
uint32_t blockCount = 0;

//To create thread safety for memory allocation
DECLARE_LOCK(bitmapOpperation);

inline void SetBit(uint32_t bit)
{
    memoryBitmap[bit / 32] |= (1 << (bit % 32));
}
inline void UnsetBit(uint32_t bit)
{
    memoryBitmap[bit / 32] &= ~ (1 << (bit % 32));
}
inline bool TestBit(uint32_t bit)
{
    return memoryBitmap[bit / 32] & (1 << (bit % 32));
}


uint32_t FirstFreeSize(uint32_t size)
{
    for (uint32_t i = 0; i < ARRAY_ENTRIES; i++)
        if (memoryBitmap[i] != 0xFFFFFFFF) //Completely used
            for (int j = 0; j < (ENTRIES_PER_BYTE * ARRAY_ENTRY_SIZE); j++) {
                if ((memoryBitmap[i] & (1<<j)) == 0) //Test if bit is unset
                {
                    uint32_t startingBit = i * (ENTRIES_PER_BYTE * ARRAY_ENTRY_SIZE);
                    startingBit += j; //Calculate bit offset in total array

                    uint32_t free = 0; //Loop through each bit to see if its enough space
                    for (uint32_t count = 0; count <= size; count++)
                    {
                        if (!TestBit(startingBit + count))
                            free++; // this bit is clear (free frame)

                        if (free == size)
                            return (i * ENTRIES_PER_BYTE * ARRAY_ENTRY_SIZE) + j; //free count==size needed; return index
                    }
                }
            }

    return -1;
}

void ContextHeap::Init()
{
    //Calculate amount of blocks
    blockCount = (endAddress - startAddress) / blockSize;
    
    //Allocate bitmap
    memoryBitmap = new uint32_t[ARRAY_ENTRIES];

    //All memory is free at the start
    memset(memoryBitmap, 0x00, ARRAY_ENTRIES * sizeof(uint32_t));
}
uint32_t ContextHeap::AllocateArea(uint32_t blocks)
{
    LOCK(bitmapOpperation);
    uint32_t frame = FirstFreeSize(blocks);

    if (frame == (uint32_t)-1) {
        UNLOCK(bitmapOpperation);
        return 0; //not enough space
    }

    for (uint32_t i = 0; i < blocks; i++)
        SetBit(frame + i);

    UNLOCK(bitmapOpperation);
    return startAddress + frame * blockSize; 
}
void ContextHeap::FreeArea(uint32_t addr, uint32_t blocks)
{
    addr -= startAddress;
    uint32_t frame = addr / blockSize;

    LOCK(bitmapOpperation);
    for (uint32_t i = 0; i < blocks; i++)
        UnsetBit(frame + i);
    UNLOCK(bitmapOpperation);
}
double ContextHeap::MemoryUsage()
{
    uint32_t usedCount = 0;
    for (uint32_t i = 0; i < ARRAY_ENTRIES; i++) {
        if(memoryBitmap[i] == 0xFFFFFFFF)
            usedCount += 32;
        else
            for (int j = 0; j < 32; j++) {
                if (memoryBitmap[i] & (1<<j))
                    usedCount++;
            }
    }
    return ((double)usedCount / (double)blockCount);
}