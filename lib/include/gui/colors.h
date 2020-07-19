#ifndef __LIBCACTUSOS__GUI__COLORS_H
#define __LIBCACTUSOS__GUI__COLORS_H

#include <types.h>

namespace LIBCactusOS
{
    // Union describing a ARGB color in the following format:
    // 0xAARRGGBB
    typedef union Color4Tag
    {
        uint32_t c;
        struct ColorComponents
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        } argb;
    } Color4;
  
    class Colors
    {
    public:
        /**
         * Blend to colors using alpha blending
         * Color1 is background
         * Color2 is foreground
        */
        static const uint32_t AlphaBlend(uint32_t color1, uint32_t color2);
        
        /**
         * Convert a ARGB color to 0xAARRGGBB format
        */
        static const uint32_t FromARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
    };
}

#endif