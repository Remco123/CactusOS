#ifndef __LIBCACTUSOS__GUI__CONTEXT_H
#define __LIBCACTUSOS__GUI__CONTEXT_H

#include <types.h>
#include <gui/rect.h>
#include <gui/widgets/control.h>

namespace LIBCactusOS
{    
    /**
     * Represents a region of framebuffer shared between client and server
    */
    class Context : public Rectangle
    { 
    public:
        /**
         * Which control is contained in this context.
        */
        Control* Window;

        /**
         * With this canvas you can directly draw to this context
        */
        Canvas* canvas;

        /**
         * Create a new context by a framebuffer and dimensions
        */
        Context(uint32_t framebufferAddr, int width = 0, int height = 0, int x = 0, int y = 0);
        
        /**
         * Destructor, warning: does also delete canvas
        */
        ~Context();

        /**
         * Draw all the gui components to this context
        */
        void DrawGUI();

        /*///////////////
        // Events
        *////////////////

        /**
         * Called when mouse is down on context
        */
        void OnMouseDown(int x_abs, int y_abs, uint8_t button);
        /**
         * Called when mouse is up on context
        */
        void OnMouseUp(int x_abs, int y_abs, uint8_t button);
        /**
         * Called when mouse moves above context or enters/leaves context
        */
        void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs);
    };
}

#endif