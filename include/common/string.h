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
            static int Split(const char* str, char c, char*** arr);
        };
    }
}

#endif