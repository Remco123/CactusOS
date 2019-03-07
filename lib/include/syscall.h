#ifndef __CACTUSOSLIB__SYSCALL_H
#define __CACTUSOSLIB__SYSCALL_H

namespace LIBCactusOS
{
    #define SYSCALL_RET_SUCCES 1
    #define SYSCALL_RET_ERROR 0

    #define SYSCALL_EXIT 0
    #define SYSCALL_LOG 1
    #define SYSCALL_GUI_GETLFB 2
    #define SYSCALL_FILE_EXISTS 3
    #define SYSCALL_DIR_EXISTS 4
    #define SYSCALL_GET_FILESIZE 5
    #define SYSCALL_READ_FILE 6
    #define SYSCALL_SET_CACTUSOS_LIB 0xFFFF

    int DoSyscall(unsigned int intNum, unsigned int arg1 = 0, unsigned int arg2 = 0);
}

#endif