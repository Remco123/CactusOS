#ifndef __CACTUSOS__COMMON__STRING_H
#define __CACTUSOS__COMMON__STRING_H

#include <common/types.h>
#include <common/memoryoperations.h>

namespace CactusOS
{
    namespace common
    {
        class String
        {
        public:
            static int strlen(const char* str);
        };
    }
}

#endif