#ifndef __CACTUSOS__SYSTEM__KHEAP_H
#define __CACTUSOS__SYSTEM__KHEAP_H

#include <core/virtualmemory.h>

namespace CactusOS
{
    namespace system
    {
        #define KERNEL_HEAP_START 0xC0000000
        #define KERNEL_HEAP_START_SIZE 0x100000
        #define KERNEL_HEAP_END 0xCFFFF000

        class Heap
        {
        protected:
            static common::uint32_t startAddress;
            static common::uint32_t endAddress;
            static common::uint32_t maxAddress;
        };

        class KernelHeap : public Heap
        {
            
        };
    }
}


#endif