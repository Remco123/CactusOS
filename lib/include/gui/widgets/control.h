#ifndef __LIBCACTUSOS__GUI__CONTROL_H
#define __LIBCACTUSOS__GUI__CONTROL_H

#include <types.h>
#include <gui/rect.h>
#include <list.h>
#include <gui/canvas.h>
#include <gui/events.h>
#include <gui/contextinfo.h>
#include <gui/property.h>
#include <gui/fonts/font.h>
#include <gui/colors.h>

namespace LIBCactusOS
{
    // Defines the alignment of a specific entry
    // This is for the x and the y direction
    struct Alignment
    {
        // Possible options for the alignment on the x-axis
        enum class Horizontal
        {
            Left,
            Center,
            Right
        };

        // Possible options for the alignment on the y-axis
        enum class Vertical
        {
            Top,
            Center,
            Bottom
        };

        Horizontal x; // X-Axis
        Vertical y; // Y-Axis
    };

    // Possible style options for corners of controls
    enum CornerStyle
    {
        Sharp,
        Rounded
    };

    /**
     * A class describing a gui object with possible children
    */
    class Control : public EventObject, public Rectangle
    {
    public:
        // All controls that are present on this control.
        List<Control*> childs;

        // Which control currently is focused?
        Control* focusedChild = 0;

        // If we are a child of some control this will point to our parent.
        Control* parent = 0;

        // Does this control needs to be painted again?
        bool needsRepaint = false;
        
        // Public properties for this control
        GUIProperty<uint32_t>       backColor       = GUIProperty<uint32_t>(this, 0xFF919191);
        GUIProperty<uint32_t>       borderColor     = GUIProperty<uint32_t>(this, 0xFF333333);
        GUIProperty<Font*>          font            = GUIProperty<Font*>(this, 0);
        GUIProperty<Alignment>      textAlignment   = GUIProperty<Alignment>(this, { Alignment::Horizontal::Left, Alignment::Vertical::Top });
        GUIProperty<uint32_t>       textColor       = GUIProperty<uint32_t>(this, Colors::Black);
        GUIProperty<CornerStyle>    cornerStyle     = GUIProperty<CornerStyle>(this, CornerStyle::Sharp);
        GUIProperty<uint16_t>       cornerRadius    = GUIProperty<uint16_t>(this, 5);

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
        virtual ~Control();

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

        // Force this control to be drawn aggain
        virtual void ForcePaint();

        // Return the visual portion of this control in aspect with the parent
        virtual Rectangle GetParentsBounds(int xOffset, int yOffset);

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
         * Called when mouse enters control
        */
        virtual void OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs);
        /**
         * Called when mouse leaves control
        */
        virtual void OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs);
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
        /**
         * Called when there is a scroll event on control
        */
        virtual void OnScroll(int32_t deltaZ, int x_abs, int y_abs);
    };
}

#endif