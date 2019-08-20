#ifndef __LIBCACTUSOS__GUI__EVENTS_H
#define __LIBCACTUSOS__GUI__EVENTS_H

#include <list.h>
#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    // Interface for event arguments
    class IEventArgs {};

    // A class that defines a callback function 
    class IEventCallback
    {
    public:
        virtual void Invoke(void*, IEventArgs) = 0;
    };

    // A callback that is a class method
    template<typename T>
    class MethodCallback : public IEventCallback
    {
    private:
        void (T::*function)(void* s, IEventArgs arg);
        T* instance;
    public:
        // Create a new callback based on a member function
        MethodCallback(T* instance, void (T::*function)(void* s, IEventArgs arg))
        : instance(instance), function(function) 
        {}

        virtual void Invoke(void* s, IEventArgs arg) override
        {
            (instance->*function)(s, arg); 
        }
    };

    // A callback that is a static function
    class StaticFunctionCallback : public IEventCallback
    {
    private:
        void (*func_)(void*, IEventArgs);
    
    public:
        StaticFunctionCallback(void (*func)(void*, IEventArgs))
        : func_(func)
        {}
        
        virtual void Invoke(void* s, IEventArgs a)
        {
            return (*func_)(s, a);
        }
    };

    // A class that manages a list of multipile callback for a specific event
    template<typename Args>
    class EventHandlerList
    {
    private:
	    List<IEventCallback*> Callbacks;
    public:
        EventHandlerList() {}
        ~EventHandlerList() {}

        void AddHandler(IEventCallback* action)
        {
            Callbacks.push_back(action);
        }
	    void RemoveHandler(IEventCallback* action)
        {
            Callbacks.Remove(action);
        }
	    void Invoke(void* sender, Args arg)
        {
            for (IEventCallback* action : Callbacks)
                action->Invoke(sender, arg);
        }

        void operator+= (IEventCallback* action)
        {
            AddHandler(action);
        }
        void operator-= (IEventCallback* action)
        {
            RemoveHandler(action);
        }
    };

////////////////////
// Event implementations for keypress or mouse events, more will be added later
////////////////////
    class MouseMoveArgs : public IEventArgs
    {
    public:
        int prevX;
        int prevY;
        int newX;
        int newY;

        MouseMoveArgs(int prevX, int prevY, int newX, int newY)
        {
            this->prevX = prevX;
            this->prevY = prevY;
            this->newX = newX;
            this->newY = newY;
        }
    };
    class MouseButtonArgs : public IEventArgs
    {
    public:
        int button;

        MouseButtonArgs(int button)
        {
            this->button = button;
        }
    };
    class KeypressArgs : public IEventArgs
    {
    public:
        char key;

        KeypressArgs(char key)
        {
            this->key = key;
        }
    };
    
    
    
    
    
    


    /**
     * An object that hosts multiple gui events
     */
    class EventObject
    {
    public:
        EventHandlerList<MouseMoveArgs> MouseMove;
        EventHandlerList<MouseButtonArgs> MouseDown;
        EventHandlerList<MouseButtonArgs> MouseUp;
        EventHandlerList<MouseButtonArgs> MouseClick;
        EventHandlerList<KeypressArgs> KeyPress;

        EventObject()
        : MouseMove(), MouseDown(), MouseUp(), MouseClick(), KeyPress()
        {        }
    };
}

#endif