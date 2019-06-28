#ifndef __LIBCACTUSOS__TIME_H
#define __LIBCACTUSOS__TIME_H

#include <types.h>

namespace LIBCactusOS
{
    class Time
    {
    public:
        /**
         * Make this thread sleep for a specific amount of ms
        */
        static void Sleep(uint32_t ms);
        /**
         * Get the current amount of ticks from the timer
        */
        static uint64_t Ticks();
    };
}

#endif