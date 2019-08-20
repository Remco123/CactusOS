#ifndef __LIBCACTUSOS__GUI__EVENTS_H
#define __LIBCACTUSOS__GUI__EVENTS_H

#include <list.h>
#include <delegates.h>
#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    class Control;
    class EventArgs
    {
    public:
        Control* sender;
        
        EventArgs(Control* source)
        {
            this->sender = source;
        }
    };

    class EventHandlerList
    {
    private:
        List<Delegate<void, EventArgs>> callbacks;
    public:
        /**
         * Create new EventHandlerList 
        */
        EventHandlerList()
        : callbacks()
        {
            callbacks.Clear();
        }

        void AddHandler(Delegate<void, EventArgs> handler)
        {
            callbacks.push_back(handler);
        }
        void operator+=(Delegate<void, EventArgs> handler)
        {
            AddHandler(handler);
        }
        /**
         * Call all handlers for this event
        */
        void operator()(EventArgs arg)
        {
            for(auto c : callbacks)
                c(arg);
        }
    };

    /**
     * An object that hosts multiple gui events
     */
    class EventObject
    {
    public:
        EventHandlerList MouseMove;
        EventHandlerList MouseDown;
        EventHandlerList MouseUp;
        EventHandlerList MouseClick;
        EventHandlerList KeyPress;

        EventObject()
        : MouseMove(), MouseDown(), MouseUp(), MouseClick(), KeyPress()
        {        }
    };
}

#endif