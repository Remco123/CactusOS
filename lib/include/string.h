#ifndef __CACTUSOSLIB__MEMORYOPERATIONS_H
#define __CACTUSOSLIB__MEMORYOPERATIONS_H

#include <stddef.h>

extern "C"
{
    int memcmp(const void*, const void*, size_t);
    void* memcpy(void* __restrict, const void* __restrict, size_t);
    void* memmove(void*, const void*, size_t);
    void* memset(void*, int, size_t);
    size_t strlen(const char*);
}

#endif