#ifndef __LIBCACTUSOS__GUI__EVENTS_H
#define __LIBCACTUSOS__GUI__EVENTS_H

#include <list.h>
#include <delegates.h>

namespace LIBCactusOS
{
    class EventHandlerList
    {
    private:
        List<Delegate<void>> callbacks;
    public:
        /**
         * Create new EventHandlerList 
        */
        EventHandlerList()
        : callbacks()
        {
            callbacks.Clear();
        }

        void AddHandler(Delegate<void> handler)
        {
            callbacks.push_back(handler);
        }
        void operator+=(Delegate<void> handler)
        {
            AddHandler(handler);
        }
        /**
         * Call all handlers for this event
        */
        void operator()()
        {
            for(auto c : callbacks)
                c();
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