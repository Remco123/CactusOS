#include <vector.h>
#include <string.h>

using namespace LIBCactusOS;

template<typename T>
void Vector<T>::reserve(int capacity)
{
    T* newBuf = new T[capacity];
    memcpy(newBuf, this->buffer, sizeof(T) * this->size);

    this->capacity = capacity;
    
    delete this->buffer;
    this->buffer = newBuf;
}

template<typename T>
Vector<T>::Vector()
{
    this->size = 0;
    this->capacity = 0;
    this->buffer = 0;
}

template<typename T>
int Vector<T>::Size()
{
    return this->size;
}

template<typename T>
void Vector<T>::push_back(const T& item)
{
    if(this->capacity == 0)
        reserve(10);
    else if(this->size == this->capacity)
        reserve(2 * this->size);
    
    this->buffer[this->size] = item;
    this->size++;
}

template<typename T>
void Vector<T>::pop_back()
{
    this->size--;
}

template<typename T>
void Vector<T>::clear()
{
    this->capacity = 0;
    this->size = 0;

    delete this->buffer;
    this->buffer = 0;
}

template<typename T>
T& Vector<T>::GetAt(int n)
{
    return this->buffer[n];
}

template<typename T>
T& Vector<T>::operator[](int n)
{
    return this->buffer[n];
}

template<typename T>
T* Vector<T>::data()
{
    return this->buffer;
}