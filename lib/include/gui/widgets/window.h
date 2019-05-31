#ifndef __LIBCACTUSOS__WINDOW_H
#define __LIBCACTUSOS__WINDOW_H

#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    class Window : public Control
    {
    private:
        uint32_t titleBarColor = 0xFF4CB272;
        uint16_t titleBarHeight = 30;
    public:
        char* titleString = 0;
        /**
         * Create a new window with width and height
        */
        Window(uint32_t w, uint32_t h);
        /**
         * Create a new window with width, height, x and y
        */
        Window(uint32_t w, uint32_t h, uint32_t x, uint32_t y);

        /**
         * Draw this window to a canvas
         * 
         * x_abs/y_abs: the coördinate of this window in absolute related to the canvas
        */
        void DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs);

        /**
         * Called when mouse is down on window
        */
        void OnMouseDown(uint32_t x_abs, uint32_t y_abs, uint8_t button);
        /**
         * Called when mouse is up on window
        */
        void OnMouseUp(uint32_t x_abs, uint32_t y_abs, uint8_t button);
    };
}

#endif