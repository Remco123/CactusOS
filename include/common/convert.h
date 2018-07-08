#ifndef CACTUSOS__COMMON__CONVERT_H
#define CACTUSOS__COMMON__CONVERT_H

#include <common/types.h>

namespace CactusOS
{
    namespace common
    {
        class Convert
        {
        private:
            static int isspace(char c);
        public:
            static char* IntToString(int i);
            static char* IntToString(char *buf, unsigned long int n, int base);
            static int StringToInt(char* string);
            static char* CharToString(char c);
            static common::uint16_t ByteSwap(common::uint16_t key);
            static common::uint32_t ByteSwap(common::uint32_t key);
        };
    }
}

#endif