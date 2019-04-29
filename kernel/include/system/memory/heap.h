#ifndef __CACTUSOS__SYSTEM__HEAP_H
#define __CACTUSOS__SYSTEM__HEAP_H

#include <core/virtualmemory.h>

namespace CactusOS
{
    namespace system
    {
        #define KERNEL_HEAP_START 0xC0400000
        #define KERNEL_HEAP_START_SIZE 0x100000
        #define KERNEL_HEAP_END 0xCFFFF000

        #define USE_HEAP_MAGIC 1
        
        #if USE_HEAP_MAGIC
        #define MEMORY_HEADER_MAGIC 0xF00D
        #endif

        #ifndef align_up
        #define align_up(num, align) \
            (((num) + ((align) - 1)) & ~((align) - 1))
        #endif

        struct MemoryHeader
        {
            #if USE_HEAP_MAGIC
            common::uint32_t magic;
            #else
            common::uint32_t reserved;
            #endif

            MemoryHeader* next;
            MemoryHeader* prev;
            bool allocated;
            common::uint32_t size;
        } __attribute__((packed));

        class KernelHeap
        {
        private:
            static common::uint32_t startAddress;
            static common::uint32_t endAddress;
            static common::uint32_t maxAddress;

            static MemoryHeader* firstHeader;

            static void* InternalAllocate(common::uint32_t size);
            static void InternalFree(void* ptr);
        public:
            static bool Enabled;
            static void Initialize(common::uint32_t start, common::uint32_t end, common::uint32_t max);

            static void* malloc(common::uint32_t size, common::uint32_t* physReturn = 0);
            static void free(void* ptr);

            static void* allignedMalloc(common::uint32_t size, common::uint32_t align, common::uint32_t* physReturn = 0);
            static void allignedFree(void* ptr);

            #if USE_HEAP_MAGIC
            static bool CheckForErrors();
            #endif
        };
    }
}


#endif