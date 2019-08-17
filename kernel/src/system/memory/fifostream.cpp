#include <system/memory/fifostream.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

FIFOStream::FIFOStream(int capacity)
{
    this->buffer = new char[capacity * sizeof(char)];
    this->buffer_end = (char*)this->buffer + capacity * sizeof(char);
    this->count = 0;
    this->capacity = capacity;
    this->head = this->buffer;
    this->tail = this->buffer;
}

FIFOStream::~FIFOStream()
{
    delete this->buffer;
}

void FIFOStream::Write(char item)
{
    if(this->count == this->capacity)
        return;

    MemoryOperations::memcpy((void*)this->head, (void*)&item, sizeof(char));
    this->head = (char*)(this->head + sizeof(char));
    if(this->head == this->buffer_end)
        this->head = this->buffer;
    
    this->count++;
}

char FIFOStream::Read()
{
    char result;
    if(this->count == 0)
        return result;

    MemoryOperations::memcpy((void*)&result, (void*)this->tail, sizeof(char));
    this->tail = (char*)(this->tail + sizeof(char));
    if(this->tail == this->buffer_end)
        this->tail = this->buffer;

    this->count--;
    return result;
}

int FIFOStream::Availible()
{
    return this->count;
}