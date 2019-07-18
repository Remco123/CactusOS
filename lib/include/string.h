#ifndef __CACTUSOSLIB__MEMORYOPERATIONS_H
#define __CACTUSOSLIB__MEMORYOPERATIONS_H

#include <stddef.h>
#include <list.h>

extern "C"
{
    int memcmp(const void* aptr, const void* bptr, size_t size);
    void* memcpy(void* __restrict__ dstptr, const void* __restrict__ srcptr, size_t size);
    void* memmove(void* dstptr, const void* srcptr, size_t size);
    void* memset(void* bufptr, int value, size_t size);
    size_t strlen(const char* str);
   
    int str_IndexOf(const char* str, char c, int skip = 0);
    bool str_Contains(const char* str, char c);
    List<char*> str_Split(const char* str, char d);
    char* str_Uppercase(char* str);
    char* str_Lowercase(char* str);
}

#endif