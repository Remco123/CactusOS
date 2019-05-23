#ifndef __LIBCACTUSOS__GUI__RECT_H
#define __LIBCACTUSOS__GUI__RECT_H

#include <types.h>

namespace LIBCactusOS
{
    class Rectangle
    {
    public:
        uint32_t width;
        uint32_t height;
        uint32_t x;
        uint32_t y;
    
        Rectangle(uint32_t w, uint32_t h, uint32_t x = 0, uint32_t y = 0);
    };
}

#endif