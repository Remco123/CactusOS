#ifndef __CACTUSOS__CORE__MEMORYMANAGEMENT_H
#define __CACTUSOS__CORE__MEMORYMANAGEMENT_H

#include <common/types.h>
#include <stddef.h>

typedef CactusOS::common::uint16_t offset_t;

#ifndef align_up
#define align_up(num, align) \
    (((num) + ((align) - 1)) & ~((align) - 1))
#endif

namespace CactusOS
{
    namespace core
    {
        struct MemoryChunk
        {
            MemoryChunk *next;
            MemoryChunk *prev;
            bool allocated;
            common::size_t size;
        };
         
        class MemoryManager
        {   
        protected:
            MemoryChunk* first;
        public:
            static MemoryManager *activeMemoryManager;
            MemoryManager(common::size_t first, common::size_t size);
            ~MemoryManager();
            
            void* malloc(common::size_t size);
            void free(void* ptr);
            void* aligned_malloc(common::size_t align, common::size_t size);
            void aligned_free(void* ptr);
        };
    }
}

void* operator new(size_t size);
void* operator new[](size_t size);

// placement new
void* operator new(size_t size, void* ptr);
void* operator new[](size_t size, void* ptr);

void operator delete(void* ptr);
void operator delete[](void* ptr);

void operator delete(void* ptr, unsigned long size);
void operator delete[](void* ptr, unsigned long size);

#endif