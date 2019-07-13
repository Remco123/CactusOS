#ifndef __LIBCACTUSOS__GUI__COLORS_H
#define __LIBCACTUSOS__GUI__COLORS_H

#include <types.h>

namespace LIBCactusOS
{
    class Colors
    {
    public:
        /**
         * Blend to colors using alpha blending
         * Color1 is background
         * Color2 is foreground
        */
        static uint32_t AlphaBlend(uint32_t color1, uint32_t color2);
    };
}

#endif