#ifndef __LIBCACTUSOS__WINDOW_H
#define __LIBCACTUSOS__WINDOW_H

#include <gui/widgets/control.h>
#include <gui/widgets/button.h>
#include <gui/context.h>
#include <gui/properties.h>

namespace LIBCactusOS
{
    class Window : public Control
    {
    private:
        GUI_PROPERTY_DEC(TitleBarColor, uint32_t, 0xFF1E9AFF, true);
        GUI_PROPERTY_DEC(TitleBarHeight, uint16_t, 30, true);

        // Is the mouse down on the title bar?
        bool titleBarMouseDown = false;

        // Remember where the mouse was held down for a smooth window drag
        int mouseDownX = 0;
        int mouseDownY = 0;

        // Window Control Buttons
        Button* closeButton = 0;

        // Create the close button for this window
        void CreateButtons();
    public:
        char* titleString = 0;

        /**
         * In which context are we located?
        */
        Context* contextBase = 0;

        /**
         * Create a new window with a context as base
        */
        Window(Context* base);

        /**
         * Create a new window that request a context for itself to use
        */
        Window(int width, int height, int x, int y);
        
        /**
         * Close this window, this can cause the application to exit
        */
        void Close(void* sender, MouseButtonArgs arg);
        /**
         * Close this window, this can cause the application to exit
        */
        void Close();

        /**
         * Draw this window to a canvas
         * 
         * x_abs/y_abs: the coördinate of this window in absolute related to the canvas
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs);
    friend class Context;
    protected:
        /**
         * Called when mouse is down on window
        */
        void OnMouseDown(int x_abs, int y_abs, uint8_t button) override;
        /**
         * Called when mouse is up on window
        */
        void OnMouseUp(int x_abs, int y_abs, uint8_t button) override;
        /**
         * Called when mouse is moved on window
        */
        void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;
        /**
         * Called when Window is resized
        */
        void OnResize(Rectangle old) override;
        /**
         * Called when key is held down
        */
        void OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers) override;
    };
}

#endif