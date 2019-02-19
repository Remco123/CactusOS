#ifndef __CACTUSOSLIB__SYSCALL_H
#define __CACTUSOSLIB__SYSCALL_H

#include <../../kernel/include/common/types.h>

namespace LIBCactusOS
{
    #define SYSCALL_LOG 12
    #define SYSCALL_SET_CACTUSOS_LIB 0xFFFF

    int DoSyscall(CactusOS::common::uint32_t intNum, CactusOS::common::uint32_t arg1 = 0, CactusOS::common::uint32_t arg2 = 0);
}

#endif