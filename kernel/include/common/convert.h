#ifndef CACTUSOS__COMMON__CONVERT_H
#define CACTUSOS__COMMON__CONVERT_H

#include <common/types.h>
#include <common/memoryoperations.h>

namespace CactusOS
{
    namespace common
    {
        class Convert
        {
        public:
            static char* IntToString(common::uint32_t i);

            static char* IntToHexString(common::uint8_t w);
        };
    }
}

#endif