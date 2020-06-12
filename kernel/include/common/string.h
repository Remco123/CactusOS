#ifndef __CACTUSOS__COMMON__STRING_H
#define __CACTUSOS__COMMON__STRING_H

#include <common/types.h>
#include <common/list.h>
#include <common/memoryoperations.h>

namespace CactusOS
{
    namespace common
    {
        #define isalpha(c) (((unsigned)c|32)-'a' < 26)

        class String
        {
        public:
            static int strlen(const char* str);
            static bool strcmp(const char* strA, const char* strB);
            static bool strncmp(const char* s1, const char* s2, int n);
            static int IndexOf(const char* str, char c, common::uint32_t skip = 0);
            static bool Contains(const char* str, char c);
            static List<char*> Split(const char* str, char d);
            static char* Uppercase(char* str);
            static char* Lowercase(char* str);
            static char Uppercase(char c);
            static char Lowercase(char c);
            static char* strcpy(char *s1, const char *s2);
            static char* strncpy(char *s1, const char *s2, unsigned int n);
        };
    }
}

#endif