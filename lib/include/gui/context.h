#ifndef __LIBCACTUSOS__GUI__CONTEXT_H
#define __LIBCACTUSOS__GUI__CONTEXT_H

#include <types.h>
#include <gui/rect.h>
#include <gui/contextinfo.h>
#include <gui/widgets/control.h>

namespace LIBCactusOS
{   
    typedef void (*GUI_MouseCall) (Control* sender, uint8_t button);  

    /**
     * Represents a region of framebuffer shared between client and server
    */
    class Context
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
         * A struct that is shared with the compositor that describes the physical dimensions of this context
        */
        ContextInfo* sharedContextInfo;

        /**
         * A pointer to a function that gets called when the mouse is clicked inside this context
        */
        GUI_MouseCall mouseClickHandler = 0;

        /**
         * Create a new context by a framebuffer and dimensions
        */
        Context(uint32_t framebufferAddr, int width = 0, int height = 0);
        
        /**
         * Destructor, warning: does also delete canvas
        */
        ~Context();

        /**
         * Draw all the gui components to this context
        */
        void DrawGUI();

        /**
         * Move this context to a new position.
        */
        void MoveToPosition(int newX, int newY);

        /**
         * Remove this context from the screen and free all the used memory 
        */
        void CloseContext();

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
        /**
         * Called when a key is pressed and this is the active context.
        */
        void OnKeyPress(char key);
    };
}

#endif