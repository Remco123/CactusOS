#ifndef __CACTUSOS__COMMON__MEMORYOPERATIONS_H
#define __CACTUSOS__COMMON__MEMORYOPERATIONS_H

#include <common/types.h>

namespace CactusOS
{
    namespace common
    {
        #define phys2virt(x) ((x) + 0xC0000000)
        #define virt2phys(x) ((x) - 0xC0000000)

        class MemoryOperations
        {
        public:
            static void* memmove(void* dstptr, const void* srcptr, uint32_t size);
            static int memcmp(const void* aptr, const void* bptr, uint32_t size);
            static void* memset(void* bufptr, char value, uint32_t size);
            static void* memcpy(void* dstptr, const void* srcptr, uint32_t size);
        };
    }
}

#endif