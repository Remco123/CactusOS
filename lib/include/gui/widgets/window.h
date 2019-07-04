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

        /**
         * In which context are we located?
        */
        Context* contextBase = 0;
    public:
        char* titleString = 0;

        /**
         * Create a new window with a context as base
        */
        Window(Context* base);

        /**
         * Create a new window that request a context for itself to use
        */
        Window(int width, int height, int x, int y);

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