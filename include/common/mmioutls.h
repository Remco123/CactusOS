#ifndef __CACTUSOS__COMMON__MMIOUTILS_H
#define __CACTUSOS__COMMON__MMIOUTILS_H

#include <common/types.h>

namespace CactusOS
{
    namespace common
    {
        class MMIOUtils
        {
            public:
                static uint8_t read8 (uint64_t p_address);
                static uint16_t read16 (uint64_t p_address);
                static uint32_t read32 (uint64_t p_address);
                static uint64_t read64 (uint64_t p_address);
                static void write8 (uint64_t p_address,uint8_t p_value);
                static void write16 (uint64_t p_address,uint16_t p_value);
                static void write32 (uint64_t p_address,uint32_t p_value);
                static void write64 (uint64_t p_address,uint64_t p_value);
        };
    }
}

#endif