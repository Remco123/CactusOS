#ifndef __LIBCACTUSOS__GUI__CONTROL_H
#define __LIBCACTUSOS__GUI__CONTROL_H

#include <types.h>
#include <gui/rect.h>
#include <list.h>
#include <gui/canvas.h>

namespace LIBCactusOS
{
    /**
     * A class describing a gui object with possible children
    */
    class Control : public Rectangle
    {
    public:
        /**
         * Are we a child of some control? This points to our parent
        */
        Control* parent;
        /**
         * Our list of childs
        */
        List<Control*> childs;

        uint32_t backColor = 0xFFCACDD1;
        uint32_t borderColor = 0xFF333333;

        /**
         * Create new control with a given width and height
        */
        Control(uint32_t w, uint32_t h);
        /**
         * Create new control with a given width, height, x and y position
        */
        Control(uint32_t w, uint32_t h, uint32_t x, uint32_t y);
        /**
         * Destructor
        */
        ~Control();

        /**
         * Draw this control to a canvas
         * 
         * x_abs/y_abs: the coördinate of this control in absolute related to the canvas
        */
        virtual void DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs);
    };
}

#endif