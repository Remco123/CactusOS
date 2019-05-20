#ifndef __CACTUSOSLIB__MEMORYOPERATIONS_H
#define __CACTUSOSLIB__MEMORYOPERATIONS_H

#include <stddef.h>

extern "C"
{
    int memcmp(const void* aptr, const void* bptr, size_t size);
    void* memcpy(void* __restrict__ dstptr, const void* __restrict__ srcptr, size_t size);
    void* memmove(void* dstptr, const void* srcptr, size_t size);
    void* memset(void* bufptr, int value, size_t size);
    size_t strlen(const char* str);
}

#endif