#ifndef __LIBCACTUSOS__GUI__EVENTS_H
#define __LIBCACTUSOS__GUI__EVENTS_H

#include <list.h>
#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    // A class that defines a callback function 
    template<typename ArgumentType>
    class IEventCallback
    {
    public:
        virtual void Invoke(void*, ArgumentType) = 0;
    };

    // A callback that is a class method
    template<typename T, typename ArgumentType>
    class MethodCallback : public IEventCallback<ArgumentType>
    {
    private:
        T* instance;
        void (T::*function)(void* s, ArgumentType arg);
    public:
        // Create a new callback based on a member function
        MethodCallback(T* instance, void (T::*function)(void* s, ArgumentType arg))
        : instance(instance), function(function) 
        {}

        virtual void Invoke(void* s, ArgumentType arg) override
        {
            (instance->*function)(s, arg); 
        }
    };

    // A callback that is a static function
    template<typename ArgumentType>
    class StaticFunctionCallback : public IEventCallback<ArgumentType>
    {
    private:
        void (*func_)(void*, ArgumentType);
    
    public:
        StaticFunctionCallback(void (*func)(void*, ArgumentType))
        : func_(func)
        {}
        
        virtual void Invoke(void* s, ArgumentType a)
        {
            return (*func_)(s, a);
        }
    };

    // A class that manages a list of multipile callback for a specific event
    template<typename ArgumentType>
    class EventHandlerList
    {
    private:
	    List<IEventCallback<ArgumentType>*> Callbacks;
    public:
        EventHandlerList() {}
        ~EventHandlerList() {}

        void AddHandler(IEventCallback<ArgumentType>* action)
        {
            Callbacks.push_back(action);
        }
	    void RemoveHandler(IEventCallback<ArgumentType>* action)
        {
            Callbacks.Remove(action);
            delete action;
        }
	    void Invoke(void* sender, ArgumentType arg)
        {
            for (IEventCallback<ArgumentType>* action : Callbacks)
                action->Invoke(sender, arg);
        }

        void operator+= (IEventCallback<ArgumentType>* action)
        {
            AddHandler(action);
        }
        void operator+= (void (*func)(void*, ArgumentType))
        {
            AddHandler(new StaticFunctionCallback<ArgumentType>(func));
        }
        void operator-= (IEventCallback<ArgumentType>* action)
        {
            RemoveHandler(action);
        }
    };

////////////////////
// Event implementations for keypress or mouse events, more will be added later
////////////////////
    class MouseMoveArgs
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
    class MouseButtonArgs
    {
    public:
        int button;

        MouseButtonArgs(int button)
        {
            this->button = button;
        }
    };
    class KeypressArgs
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
        EventHandlerList<MouseButtonArgs> MouseDown;
        EventHandlerList<MouseButtonArgs> MouseUp;
        EventHandlerList<MouseButtonArgs> MouseClick;
        EventHandlerList<KeypressArgs> KeyPress;

        EventObject()
        : MouseDown(), MouseUp(), MouseClick(), KeyPress()
        {        }
    };
}

#endif