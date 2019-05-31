#ifndef __LIBCACTUSOS__GUI__GUI_H
#define __LIBCACTUSOS__GUI__GUI_H

#include <gui/control.h>

namespace LIBCactusOS
{
    #define EVENTTYPE_MOUSEDOWN 0
    #define EVENTTYPE_MOUSEUP 1

    class GUI
    {
    friend class Control;
    friend class Window;
    protected:
        static List<Control*>* Windows; //All the windows associated with this program
    private:
        static Control* FindTargetControl(int m_x, int m_y);
    public:
        /**
         * Initialize the gui for this process
        */
        static void Initialize();
        /**
         * Process all the possible gui events
         * Returns if the application should continue running
        */
        static bool ProcessEvents();
    };
}

#endif