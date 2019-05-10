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
    #define SYSCALL_GET_HEAP_START 7
    #define SYSCALL_GET_HEAP_END 8
    #define SYSCALL_PRINT 9
    #define SYSCALL_SET_HEAP_SIZE 10
    #define SYSCALL_RUN_PROC 11
    #define SYSCALL_SLEEP_MS 12
    #define SYSCALL_CREATE_SHARED_MEM 13
    #define SYSCALL_SET_CACTUSOS_LIB 0xFFFF

    int DoSyscall(unsigned int intNum, unsigned int arg1 = 0, unsigned int arg2 = 0, unsigned int arg3 = 0);
}

#endif