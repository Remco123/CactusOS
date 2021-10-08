#ifndef __LIBCACTUSOS__GUI__EVENTS_H
#define __LIBCACTUSOS__GUI__EVENTS_H

#include <list.h>
#include <gui/widgets/control.h>
#include <shared.h>

namespace LIBCactusOS
{
    // A class that defines a callback function 
    template<typename ArgumentType>
    class IEventCallback
    {
    public:
        virtual void Invoke(void*, ArgumentType) {}
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
    // Argument macro implementations
    ////////////////////
    #define CREATE_ARGUMENT_CLASS0(name) \
        class name \
        { \
        public: \
            name() {} \
        }; \
    
    #define CREATE_ARGUMENT_CLASS1(name, t1, var1) \
        class name \
        { \
        public: \
            t1 var1; \
            name(t1 var1) \
            { \
                this->var1 = var1; \
            } \
        }; \
    
    #define CREATE_ARGUMENT_CLASS2(name, t1, var1, t2, var2) \
        class name \
        { \
        public: \
            t1 var1; \
            t2 var2; \
            name(t1 var1, t2 var2) \
            { \
                this->var1 = var1; \
                this->var2 = var2; \
            } \
        }; \
    
    #define CREATE_ARGUMENT_CLASS3(name, t1, var1, t2, var2, t3, var3) \
        class name \
        { \
        public: \
            t1 var1; \
            t2 var2; \
            t3 var3; \
            name(t1 var1, t2 var2, t3 var3) \
            { \
                this->var1 = var1; \
                this->var2 = var2; \
                this->var3 = var3; \
            } \
        }; \
    
    #define CREATE_ARGUMENT_CLASS4(name, t1, var1, t2, var2, t3, var3, t4, var4) \
        class name \
        { \
        public: \
            t1 var1; \
            t2 var2; \
            t3 var3; \
            t4 var4; \
            name(t1 var1, t2 var2, t3 var3, t4 var4) \
            { \
                this->var1 = var1; \
                this->var2 = var2; \
                this->var3 = var3; \
                this->var4 = var4; \
            } \
        }; \

    ///////////
    // Argument classes for keypress or mouse events, more will be added later
    ///////////
    CREATE_ARGUMENT_CLASS4(MouseMoveArgs, int, prevX, int, prevY, int, newX, int, newY)
    CREATE_ARGUMENT_CLASS3(MouseButtonArgs, int, x, int, y, int, button)
    CREATE_ARGUMENT_CLASS2(KeypressArgs, uint8_t, key, KEYPACKET_FLAGS, modifiers)
    CREATE_ARGUMENT_CLASS3(MouseScrollArgs, int, delta, int, x, int, y);

    /**
     * An object that hosts multiple gui events
     */
    class EventObject
    {
    public:
        EventHandlerList<MouseButtonArgs> MouseDown;
        EventHandlerList<MouseButtonArgs> MouseUp;
        EventHandlerList<MouseButtonArgs> MouseClick;
        EventHandlerList<KeypressArgs> KeyDown;
        EventHandlerList<KeypressArgs> KeyUp;
        EventHandlerList<MouseScrollArgs> MouseScroll;

        EventObject()
        : MouseDown(), MouseUp(), MouseClick(), KeyDown(), KeyUp(), MouseScroll()
        {        }
    };
}

#endif