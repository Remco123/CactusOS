#ifndef __LIBCACTUSOS__GUI__CONTEXTHEAP_H
#define __LIBCACTUSOS__GUI__CONTEXTHEAP_H

#include <types.h>

namespace LIBCactusOS
{
    //Class that provides functions for allocating memory space for contexts.
    //Note: Memory region is not allocated, just a block that is reserved after allocating.
    class ContextHeap
    {
    public:
        static void Init();
        //Allocate a area of memory, blocks is in units per 4096 bytes
        static uint32_t AllocateArea(uint32_t blocks);
        //Free area of memory
        static void FreeArea(uint32_t address, uint32_t blocks);
        //Get amount of memory used as factor [0-1]
        static double MemoryUsage();
    };
}

#endif