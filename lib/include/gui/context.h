#ifndef __LIBCACTUSOS__GUI__CONTEXT_H
#define __LIBCACTUSOS__GUI__CONTEXT_H

#include <types.h>
#include <gui/rect.h>
#include <gui/contextinfo.h>
#include <gui/widgets/control.h>
#include <shared.h>

namespace LIBCactusOS
{   
    /**
     * Represents a region of framebuffer shared between client and server
    */
    class Context : public EventObject
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
         * Create a new context by a framebuffer and dimensions
        */
        Context(uint32_t framebufferAddr, int width = 0, int height = 0);

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

        /**
         * Function to draw a peice of text aligned within a boundry
        */
        static void DrawStringAligned(Canvas* target, Font* font, char* string, uint32_t color, Rectangle bounds, Alignment align, int xoff = 0, int yoff = 0);

    /*///////////////
    // Events called by GUI class
    *////////////////
    friend class GUI;
    protected:
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
         * Called when a key is held down and this is the active context.
        */
        void OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers);
        /**
         * Called when a key is held up and this is the active context.
        */
        void OnKeyUp(uint8_t key, KEYPACKET_FLAGS modifiers);
        /**
         * Called when contex is resized
        */
        void OnResize(Rectangle old);
        /**
         * Called when scroll wheel is used on context
        */
        void OnScroll(int32_t deltaZ, int x_abs, int y_abs);
    };
}

#endif