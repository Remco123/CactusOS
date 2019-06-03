#ifndef __LIBCACTUSOS__GUI__GUI_H
#define __LIBCACTUSOS__GUI__GUI_H

#include <gui/widgets/control.h>
#include <gui/context.h>

namespace LIBCactusOS
{
    //Event definitions
    #define EVENT_TYPE_MOUSEDOWN 0
    #define EVENT_TYPE_MOUSEUP 1
    
    //Communication to the compositor definitions
    #define COMPOSITOR_REQUESTCONTEXT 1

    typedef void (*GUI_MouseCall) (Control* sender, uint8_t button); 

    class GUI
    {
    private:
        static Context* FindTargetContext(int m_x, int m_y);
        
        /**
         * The address whera a new context will be mapped to
        */
        static uint32_t curVirtualFramebufferAddress;
    public:
        /**
         * The PID used by the compositor process
        */
        static int compositorPID;

        /**
         * The list of all contexts in this application
        */
        static List<Context*>* contextList;

        /**
         * Initalize the gui for this process
        */
        static void Initialize();
        
        /**
         * Process all the possible gui events
        */
        static void ProcessEvents();

        /**
         * Draw all the contexts to the screen
        */
        static void DrawGUI();

        /**
         * Request a context buffer for the application to draw to, this buffer is shared between the process and the compositor
         * This buffer can be used for a gui but also for raw drawing to the screen
         * 
         * returns a pointer to the context struct
         * @param width The width of the context
         * @param height The height of the context
         * @param x The position on horizontal axis
         * @param y The position on vertical axis
        */
        static Context* RequestContext(int width, int height, int x, int y);
    };
}

#endif