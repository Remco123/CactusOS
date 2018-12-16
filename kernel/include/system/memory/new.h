#ifndef __CACTUSOS__SYSTEM__NEW_H
#define __CACTUSOS__SYSTEM__NEW_H

#include <stddef.h>
#include <system/memory/heap.h>

using namespace CactusOS::system;
 
void *operator new(size_t size)
{
    return KernelHeap::malloc(size);
}
 
void *operator new[](size_t size)
{
    return KernelHeap::malloc(size);
}
 
void operator delete(void *p)
{
    KernelHeap::free(p);
}
 
void operator delete[](void *p)
{
    KernelHeap::free(p);
}

#endif