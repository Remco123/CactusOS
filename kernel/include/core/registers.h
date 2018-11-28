#ifndef __CACTUSOS__CORE__REGISTERS_H
#define __CACTUSOS__CORE__REGISTERS_H

#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        struct CPUState
        {
            common::uint32_t ds;
            common::uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
            common::uint32_t int_no, err_code;
            common::uint32_t eip, cs, eflags, useresp, ss;
        } __attribute__((packed));
    }
}

#endif