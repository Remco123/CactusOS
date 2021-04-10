#ifndef __CACTUSOS__SYSTEM__MEMORY__FIFOSTREAM_H
#define __CACTUSOS__SYSTEM__MEMORY__FIFOSTREAM_H

#include <common/types.h>
#include <common/memoryoperations.h>
#include <system/memory/stream.h>

namespace CactusOS
{
    namespace system
    {
        class FIFOStream : public Stream
        {
        private:
            // Internal buffer
            char* buffer;
            // End of the internal buffer
            char* buffer_end;
            // Number of items in the buffer
            int count;
            // Maximum allowed items
            int capacity;
            // Pointer to head
            char* head;
            // Pointer to tail
            char* tail;
        public:
            /**
             * Create a new fifo stream
            */
            FIFOStream(int capacity = 100);
            ~FIFOStream();

            /**
             * Read a byte from this stream
            */
            char Read();
            /**
             * Write a byte to this stream buffer
            */
            void Write(char byte);
            /**
             * How many bytes can currently be read?
            */
            int Available();
        };
    }
}

#endif