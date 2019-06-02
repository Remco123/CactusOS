#ifndef __LIBCACTUSOS__WINDOW_H
#define __LIBCACTUSOS__WINDOW_H

#include <gui/widgets/control.h>
#include <gui/context.h>

namespace LIBCactusOS
{
    class Window : public Control
    {
    private:
        uint32_t titleBarColor = 0xFF4CB272;
        uint16_t titleBarHeight = 30;

        Context* contextParent = 0;
    public:
        char* titleString = 0;

        /**
         * Create a new window with width, height, x and y
        */
        Window(Context* parent, int w, int h, int x = 0, int y = 0);

        /**
         * Draw this window to a canvas
         * 
         * x_abs/y_abs: the coördinate of this window in absolute related to the canvas
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs);

        /**
         * Called when mouse is down on window
        */
        void OnMouseDown(int x_abs, int y_abs, uint8_t button);
        /**
         * Called when mouse is up on window
        */
        void OnMouseUp(int x_abs, int y_abs, uint8_t button);
    };
}

#endif