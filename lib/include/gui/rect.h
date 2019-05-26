#ifndef __LIBCACTUSOS__GUI__RECT_H
#define __LIBCACTUSOS__GUI__RECT_H

#include <types.h>

namespace LIBCactusOS
{
    /**
     * A class that describes a rectangulair shape
    */
    class Rectangle
    {
    public:
        /**
         * The width of this rectangle
        */
        uint32_t width;
        /**
         * The height of this rectangle
        */
        uint32_t height;
        /**
         * The x coördinate of this rectangle
        */
        uint32_t x;
        /**
         * The y coördinate of this rectangle
        */
        uint32_t y;

        /**
         * Create a new instance of the Rectangle Class
        */
        Rectangle(uint32_t w, uint32_t h, uint32_t x = 0, uint32_t y = 0);
    };
}

#endif