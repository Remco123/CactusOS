#ifndef __CACTUSOS__COMMON__RANDOM_H
#define __CACTUSOS__COMMON__RANDOM_H

#include <common/types.h>

namespace CactusOS
{
    namespace common
    {
        class Random
        {
        private:
            static uint32_t next;
        public:
            static int Next(uint32_t max = 32767);
            static int Next(uint32_t min, uint32_t max);             
            static void SetSeed(uint32_t seed);
        };
    }
}

#endif