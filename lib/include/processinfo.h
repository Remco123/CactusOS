#ifndef __LIBCACTUSOS__PROCESSINFO_H
#define __LIBCACTUSOS__PROCESSINFO_H

namespace LIBCactusOS
{
    // Holds data about a specific process, data is readonly
    struct ProcessInfo
    {
        // PID of process
        int id;
        // Which syscall interface does this process use?
        int syscallID;
        // The amount of threads of this process
        int threads;
        // Virtual memory used by heap
        unsigned int heapMemory;
        // Is this a ring 3 process?
        bool isUserspace;
        // Filename of process excecutable
        char fileName[32];
    };
}
#endif