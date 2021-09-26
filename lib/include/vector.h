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

        void reserve(int capacity)
        {
            T* newBuf = new T[capacity];
            memcpy(newBuf, this->buffer, sizeof(T) * this->size);

            this->capacity = capacity;
            
            if(this->buffer)
                delete this->buffer;
            this->buffer = newBuf;
        }
    public:
        Vector()
        {
            this->size = 0;
            this->capacity = 0;
            this->buffer = 0;
        }
        ~Vector()
        { this->clear(); }

        int Size()
        { return this->size; }

        void push_back(const T& item)
        {
            if(this->capacity == 0)
                reserve(10);
            else if(this->size == this->capacity)
                reserve(2 * this->size);
            
            this->buffer[this->size] = item;
            this->size++;
        }

        void pop_back()
        { this->size--; }

        void clear()
        {
            this->capacity = 0;
            this->size = 0;

            if(this->buffer)
                delete this->buffer;
            this->buffer = 0;
        }

        T& GetAt(int n)
        { return this->buffer[n]; }

        T& operator[](int n)
        { return this->buffer[n]; }

        T* data()
        { return this->buffer; }

        ///////////////////
        // Iterators
        ///////////////////

        typedef T* iterator;
        iterator begin()
        {
            return this->buffer;
        }

        iterator end()
        {
            return this->buffer + this->size;
        }
    };
}

#endif