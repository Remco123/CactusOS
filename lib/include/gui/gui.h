#ifndef __LIBCACTUSOS__GUI__GUI_H
#define __LIBCACTUSOS__GUI__GUI_H

#include <gui/widgets/control.h>
#include <gui/canvas.h>

namespace LIBCactusOS
{
    //Event definitions
    #define EVENT_TYPE_MOUSEDOWN 0
    #define EVENT_TYPE_MOUSEUP 1
    
    //Communication to the compositor definitions
    #define COMPOSITOR_REQUESTCONTEXT 1

    class GUI
    {
    friend class Control;
    friend class Window;
    protected:
        static List<Control*>* Windows; //All the windows associated with this program
    private:
        static Control* FindTargetControl(int m_x, int m_y);
        
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
         * Initialize the gui for this process
        */
        static void Initialize();
        /**
         * Process all the possible gui events
         * Returns if the application should continue running
        */
        static bool ProcessEvents();

        /**
         * Request a context buffer for the application to draw to, this buffer is shared between the process and the compositor
         * This buffer can be used for a gui but also for raw drawing to the screen
         * 
         * returns a pointer to the framebuffer canvas
         * @param width The width of the context
         * @param height The height of the context
        */
        static Canvas* RequestContext(int width, int height, int x, int y);
    };
}

#endif