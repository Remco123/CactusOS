#ifndef __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86ARGS_H
#define __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86ARGS_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        struct VM86Arguments
        {
            common::uint16_t AX;
            common::uint16_t BX;
            common::uint16_t CX;
            common::uint16_t DX;

            common::uint16_t DI;
        } __attribute__((packed));
    }
}

#endif