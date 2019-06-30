#ifndef __CACTUSOS__CORE__FPU_H
#define __CACTUSOS__CORE__FPU_H

#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        struct FPUControlWord
        {
            common::uint8_t InvalidOperand : 1;
            common::uint8_t DenormalOperand : 1;
            common::uint8_t ZeroDevide : 1;
            common::uint8_t Overflow : 1;
            common::uint8_t Underflow : 1;
            common::uint8_t Precision : 1;
            common::uint8_t reserved1 : 1;
            common::uint8_t reserved2 : 1;
            common::uint8_t PrecisionControl : 2;
            common::uint8_t RoundingControl : 2;
            common::uint8_t InfinityControl : 1;
            common::uint8_t reserved3 : 3;
        } __attribute__((packed));

        class FPU
        {
        public:
            static void Enable();
        };
    }
}

#endif