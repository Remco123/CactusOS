#ifndef __LIBCACTUSOS__VECTOR_H
#define __LIBCACTUSOS__VECTOR_H

#include <types.h>

namespace LIBCactusOS
{
    template<typename T>
    class Vector
    {
    private:
        uint32_t size = 0;
        uint32_t capacity = 0;
        T* buffer = 0;

        void reserve(int capacity);
    public:
        Vector();

        int Size();
        void push_back(const T& item);
        void pop_back();
        void clear();

        T& GetAt(int n);
        T& operator[](int i);
        T* data();
    };
}

#endif