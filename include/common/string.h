#ifndef __CACTUSOS__COMMON__STRING_H
#define __CACTUSOS__COMMON__STRING_H

#include <common/types.h>
#include <common/memoryoperations.h>
#include <core/memorymanagement.h>

namespace CactusOS
{
    namespace common
    {
        class String
        {
        public:
            static int strlen(const char* str);
            static void strcat(void *dest,const void *src);
            static bool strcmp(const char* strA, const char* strB);
            static bool strncmp(const char* s1, const char* s2, int n);
            static void split(const char *original, const char *delimiter, char ** & buffer, int & numStrings, int * & stringLengths); //https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
            static bool Contains(const char* str, char c);
        };
    }
}

#endif