#include <random.h>

using namespace LIBCactusOS;

uint32_t next = 12;

int Random::Next(uint32_t max)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % (max+1);
}

int Random::Next(uint32_t min, uint32_t max)
{
    return Next(max-min) + min;
}

void Random::SetSeed(uint32_t seed)
{
    next = seed;
}