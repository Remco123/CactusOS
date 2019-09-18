#ifndef LIBCACTUSOS__RANDOM_H
#define LIBCACTUSOS__RANDOM_H

#include <types.h>

namespace LIBCactusOS
{
    class Random
    {
    public:
        static int Next(uint32_t max = 32767);
        static int Next(uint32_t min, uint32_t max);             
        static void SetSeed(uint32_t seed);
    };
}

#endif