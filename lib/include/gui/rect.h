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
        int width;
        /**
         * The height of this rectangle
        */
        int height;
        /**
         * The x coördinate of this rectangle
        */
        int x;
        /**
         * The y coördinate of this rectangle
        */
        int y;

        /**
         * Create a new instance of the Rectangle Class
        */
        Rectangle(int w, int h, int x = 0, int y = 0);
    };
}

#endif