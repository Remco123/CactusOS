#ifndef __LIBCACTUSOS__PROC_H
#define __LIBCACTUSOS__PROC_H

#include <syscall.h>
#include <types.h>

namespace LIBCactusOS
{
    class Process
    {
    public:
        static int Run(const char* path);
    };
}

#endif