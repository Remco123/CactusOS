#ifndef __CACTUSOS__COMMON__PRINTF_H
#define __CACTUSOS__COMMON__PRINTF_H

#include <common/types.h>
#include <common/convert.h>
#include <system/bootconsole.h>

namespace CactusOS
{
    namespace common
    {
        class Print
        {
        public:
            static void printfHex(uint8_t key);
            static void printfHex16(uint16_t key);
            static void printfHex32(uint32_t key);
            static void printbits(uint8_t key);
            static void printbits(uint16_t key);
            static void printbits(uint32_t key);
            static void printbits(uint64_t key);
        };
    }
}

#endif