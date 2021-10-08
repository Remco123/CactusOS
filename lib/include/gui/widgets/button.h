#ifndef __LIBCACTUSOS__GUI__BUTTON_H
#define __LIBCACTUSOS__GUI__BUTTON_H

#include <gui/widgets/control.h>
#include <gui/gui.h>

namespace LIBCactusOS
{
    /**
     * A GUI button
    */
    class Button : public Control
    {
    public:
        /**
         * The text of this label
        */
        GUIProperty<char*> label = GUIProperty<char*>(this, 0);

        /**
         * Create a new button with a peice of text
        */
        Button(char* text = 0);

        /**
         * Draw this button
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs) override;

    /*/////////
    // Events
    *//////////
    friend class Window;
    friend class Context;
    protected:
        /**
         * Called when mouse is down on control
        */
        void OnMouseDown(int x_abs, int y_abs, uint8_t button) override;
        /**
         * Called when mouse is up on control
        */
        void OnMouseUp(int x_abs, int y_abs, uint8_t button) override;
        /**
         * Called when mouse enters control
        */
        void OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;
        /**
         * Called when mouse leaves control
        */
        void OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;
    };
}

#endif