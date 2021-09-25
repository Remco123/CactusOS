#ifndef __LIBCACTUSOS__BITREADER_H
#define __LIBCACTUSOS__BITREADER_H

#include <types.h>

namespace LIBCactusOS
{
    // Used to read specific bits of a datastream
    class BitReader
    {
    private:
        uint8_t* dataPtr = 0;
        uint32_t pos = 0;
        uint8_t  byte = 0;
        uint32_t numBits = 0;
    public:
        BitReader(uint8_t* data)
        {
            this->dataPtr = data;
            this->pos = 0;
            this->byte = 0;
            this->numBits = 0;
        }

        // Read single byte
        uint8_t ReadByte()
        {
            // Discard other bits
            this->numBits = 0;
            uint8_t b = this->dataPtr[this->pos];
            this->pos += 1;
            return b;
        }

        // Read single bit
        uint8_t ReadBit()
        {
            if(this->numBits <= 0) {
                this->byte = this->ReadByte();
                this->numBits = 8;
            }
            this->numBits -= 1;
            uint8_t bit = this->byte & 1;
            this->byte >>= 1;
            return bit;
        }

        // Read bits as type
        template<typename T>
        T ReadBits(uint32_t n)
        {
            T ret = 0;
            for(uint32_t i = 0; i < n; i++)
                ret |= (this->ReadBit() << i);
            
            return ret;            
        }

        // Read bytes as type
        template<typename T>
        T ReadBytes(uint32_t n)
        {
            T ret = 0;
            for(uint32_t i = 0; i < n; i++)
                ret |= (this->ReadByte() << (i*8));
            
            return ret;
        }
    };
}

#endif