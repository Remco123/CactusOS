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
         * Our list of childs
        */
        List<Control*> childs;

        Control* focusedChild;

        uint32_t backColor = 0xFFCACDD1;
        uint32_t borderColor = 0xFF333333;

        /**
         * Create new control with a given width, height, x and y position
        */
        Control(int w, int h, int x = 0, int y = 0);
        /**
         * Destructor
        */
        ~Control();

        /**
         * Draw this control to a canvas
         * 
         * x_abs/y_abs: the coördinate of this control in absolute related to the canvas
        */
        virtual void DrawTo(Canvas* context, int x_abs, int y_abs);

        /*/////////
        // Events
        *//////////

        /**
         * Called when mouse is down on control
        */
        virtual void OnMouseDown(int x_abs, int y_abs, uint8_t button);
        /**
         * Called when mouse is up on control
        */
        virtual void OnMouseUp(int x_abs, int y_abs, uint8_t button);
        /**
         * Called when mouse is moved on control
        */
        virtual void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs);
        /**
         * Called on keypress 
        */
        virtual void OnKeyPress(char key);
    };
}

#endif