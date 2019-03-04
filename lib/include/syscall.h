#ifndef __CACTUSOSLIB__SYSCALL_H
#define __CACTUSOSLIB__SYSCALL_H

#include <../../kernel/include/common/types.h>

namespace LIBCactusOS
{
    #define SYSCALL_RET_SUCCES 1
    #define SYSCALL_RET_ERROR 0

    #define SYSCALL_EXIT 0
    #define SYSCALL_LOG 1
    #define SYSCALL_GUI_GETLFB 2
    #define SYSCALL_SET_CACTUSOS_LIB 0xFFFF

    int DoSyscall(CactusOS::common::uint32_t intNum, CactusOS::common::uint32_t arg1 = 0, CactusOS::common::uint32_t arg2 = 0);
}

#endif