#ifndef __LIBCACTUSOS__PROC_H
#define __LIBCACTUSOS__PROC_H

#include <syscall.h>
#include <types.h>

namespace LIBCactusOS
{
    class Process
    {
    public:
        static int ID;
        static int Run(const char* path);
        static bool CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len);
        static bool CreateSharedMemory(int proc2ID, uint32_t virtStart1, uint32_t virtStart2, uint32_t len);
        static void CreateThread(void (*entryPoint)(), bool switchTo = false);
    };
}

#endif