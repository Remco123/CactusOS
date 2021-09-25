#ifndef __CACTUSOSLIB__HEAP_H
#define __CACTUSOSLIB__HEAP_H

#include <types.h>

namespace LIBCactusOS
{
    #ifndef align_up
    #define align_up(num, align) \
        (((num) + ((align) - 1)) & ~((align) - 1))
    #endif

    uint32_t pageRoundUp(uint32_t address);
    uint32_t pageRoundDown(uint32_t address);

    #define HEAP_INCREASE_SIZE 10_MB

    struct MemoryHeader
    {
        MemoryHeader* next;
        MemoryHeader* prev;
        bool allocated;
        uint32_t size;
    } __attribute__((packed));

    class UserHeap
    {
    private:
        static uint32_t startAddress;
        static uint32_t endAddress;
        static uint32_t maxAddress;

        static MemoryHeader* firstHeader;

    public:
        static void Initialize();
        static void PrintMemoryLayout();

        static void* Malloc(uint32_t size);
        static void Free(void* ptr);

        static void* alignedMalloc(uint32_t size, uint32_t align);
        static void allignedFree(void* ptr);
    };
}

#endif