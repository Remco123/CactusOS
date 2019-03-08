#ifndef __CACTUSOS__CORE__CPU_H
#define __CACTUSOS__CORE__CPU_H

#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        #define EDX_SSE2 (1 << 26) // Streaming SIMD Extensions 2

        class CPU
        {
        public:
            static void PrintVendor();
            static void EnableFeatures();
        };        
    }
}

#endif