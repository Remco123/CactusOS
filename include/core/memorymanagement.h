#ifndef __CACTUSOS__CORE__MEMORYMANAGEMENT_H
#define __CACTUSOS__CORE__MEMORYMANAGEMENT_H

#include <common/types.h>
#include <stddef.h>

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