#ifndef __CACTUSOS__SYSTEM__DEVICEHEAP_H
#define __CACTUSOS__SYSTEM__DEVICEHEAP_H

#include <core/virtualmemory.h>
#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        // Memory reserved for devices, 100MB Should be more than enough
        #define DEVICE_HEAP_SIZE 100_MB

        #define DEVICE_HEAP_START (KERNEL_HEAP_START + KERNEL_HEAP_SIZE + 4_MB)
        #define DEVICE_HEAP_END (DEVICE_HEAP_START + DEVICE_HEAP_SIZE)

        // Class that can allocate memory for memory mapped devices
        class DeviceHeap
        {
        private:
            // Current address of last memory allocation
            // Will increase on every allocation
            static common::uint32_t currentAddress;
        public:
            // Allocate a chunck of memory in the virtual address space
            // Must be a page aligned size
            // Note: Memory needs to be mapped to right address afterwards, function does not include this
            static common::uint32_t AllocateChunk(common::uint32_t size);
        };
    }
}


#endif