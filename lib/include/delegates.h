#ifndef __LIBCACTUSOS__DELEGATES_H
#define __LIBCACTUSOS__DELEGATES_H

///////
// Code from http://allenchou.net/2012/04/easy-c-delegates/
///////

namespace LIBCactusOS
{
    template <typename Ret>
    class Callback
    {
    public:
        virtual Ret invoke() = 0;
    };

    template <typename Ret>
    class StaticFunctionCallback : public Callback<Ret>
    {
    private:
        Ret (*func_)();
    
    public:
        StaticFunctionCallback(Ret (*func)())
        : func_(func)
        {}
        
        virtual Ret invoke()
        {
            return (*func_)();
        }
    };
    template <typename Ret, typename T, typename Method>
    class MethodCallback : public Callback<Ret>
    {
    private:
        void *object_;
        Method method_;
    
    public:
        MethodCallback(void *object, Method method)
        : object_(object)
        , method_(method)
        {}
        
        virtual Ret invoke()
        {
            T *obj = static_cast<T *>(object_);
            return (obj->*method_)();
        }
    };
    template <typename Ret>
    class Delegate
    {
    private:
        Callback<Ret> *callback_;
        
    public:
        Delegate(Ret (*func)())
        :callback_(new StaticFunctionCallback<Ret>(func))
        {}
        
        template <typename T, typename Method>
        Delegate(T *object, Method method)
        :callback_(new MethodCallback<Ret, T, Method>(object, method))
        {}
        
        ~Delegate(void) { delete callback_; }
        
        Ret operator()()
        {
            return callback_->invoke();
        }
    };
}

#endif