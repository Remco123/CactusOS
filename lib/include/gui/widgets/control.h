#ifndef __LIBCACTUSOS__GUI__CONTROL_H
#define __LIBCACTUSOS__GUI__CONTROL_H

#include <types.h>
#include <gui/rect.h>
#include <list.h>
#include <gui/canvas.h>
#include <gui/events.h>
#include <gui/contextinfo.h>

namespace LIBCactusOS
{
    /**
     * A class describing a gui object with possible children
    */
    class Control : public EventObject, public Rectangle
    {
    public:
        // All controls that are present on this control.
        List<Control*> childs;

        // Which control currently is focused?
        Control* focusedChild;

        // If we are a child of some control this will point to our parent.
        Control* parent;

        uint32_t backColor = 0xFFCACDD1;
        uint32_t borderColor = 0xFF333333;

        // Anchor of this control
        Direction anchor = Direction::None;
    public:
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

        // Add a control to the childs of this control, this will also set the parent property of the child to us.
        virtual void AddChild(Control* child, bool focus = true);

        // Remove a child from this control, does not delete the child!
        virtual void RemoveChild(Control* child);

        // Is this control focused?
        virtual bool Focused();

    /*/////////
    // Events called by parent or context
    *//////////
    friend class Window;
    friend class Context;
    protected:
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
         * Called when key is held down
        */
        virtual void OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers);
        /**
         * Called when key is held up
        */
        virtual void OnKeyUp(uint8_t key, KEYPACKET_FLAGS modifiers);
        /**
         * Called when control is resized
        */
        virtual void OnResize(Rectangle old);
    };
}

#endif