#ifndef __CACTUSOS__SYSTEM__MEMORY__STREAM_H
#define __CACTUSOS__SYSTEM__MEMORY__STREAM_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class Stream
        {
        public:
            /**
             * Create a new instance of the stream class
            */
            Stream();
            /**
             * Delete the stream and free all the memory it has used
            */
            virtual ~Stream();

            /**
             * Read a byte from this stream
            */
            virtual char Read();
            /**
             * Write a byte to this stream buffer
            */
            virtual void Write(char byte);
            /**
             * How many bytes can currently be read?
            */
            virtual int Available();
        };
    }
}

#endif