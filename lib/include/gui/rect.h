#ifndef __LIBCACTUSOS__GUI__RECT_H
#define __LIBCACTUSOS__GUI__RECT_H

#include <types.h>
#include <list.h>

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

        /**
         * Get the area of this rectangle
         * Basicly just width * height
         */
        int Area();

        /**
         * Get the intersection rectangle between this one and the target 
         * Results holds the intersected rect
         * Returns true for a intersection
        */
        bool Intersect(Rectangle other, Rectangle* result);

        /**
         * Explode this rect into a list of contiguous rects
        */
        List<Rectangle>* Split(Rectangle cuttingRect);

        /**
         * Insert this rectangle into the clip list, splitting all existing rectangles against it to prevent overlap 
        */
        void PushToClipList(List<Rectangle>* targetList);
    };
}

#endif