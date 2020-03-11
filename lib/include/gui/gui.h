#ifndef __LIBCACTUSOS__GUI__GUI_H
#define __LIBCACTUSOS__GUI__GUI_H

#include <gui/widgets/control.h>
#include <gui/context.h>

namespace LIBCactusOS
{
    //Event definitions
    #define EVENT_TYPE_MOUSEDOWN 0
    #define EVENT_TYPE_MOUSEUP 1
    #define EVENT_TYPE_MOUSEMOVE 2
    #define EVENT_TYPE_KEYPRESS 3
    #define EVENT_TYPE_RESIZE 4
    
    //Communication to the compositor definitions
    #define COMPOSITOR_REQUESTCONTEXT 1
    #define COMPOSITOR_CONTEXTMOVED 2
    #define COMPOSITOR_CONTEXTCLOSE 3

    #define MOUSE_LEFT 0
    #define MOUSE_MIDDLE 1
    #define MOUSE_RIGHT 2

    class GUI
    {
    private:
        static Context* FindTargetContext(int m_x, int m_y);
    public:
        //Current Width of video mode
        static int Width;
        //Current Height of video mode
        static int Height;
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
         * Remove all gui elements created by this process
        */
        static void CleanUp();
        
        /**
         * Process all the possible gui events
        */
        static void ProcessEvents();

        /**
         * Draw all the contexts to the screen
        */
        static void DrawGUI();

        /**
         * Create a thread for the gui
        */
        static void MakeAsync();

        /**
         * Is the current number of contexts larger than 0? 
        */
        static bool HasItems();

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