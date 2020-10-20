#ifndef __CACTUSOS__SYSTEM__DEVICEHEAP_H
#define __CACTUSOS__SYSTEM__DEVICEHEAP_H

#include <core/virtualmemory.h>
#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        // Class that can allocate memory for memory mapped devices
        // Memory will range from (KERNEL_HEAP_START+KERNEL_HEAP_SIZE) to PAGE_TABLE_ADDRESS
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
            static common::uint32_t AllocateChunck(common::uint32_t size);
        };
    }
}


#endif