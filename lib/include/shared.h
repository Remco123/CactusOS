#ifndef __LIBCACTUSOS__SHARED_H
#define __LIBCACTUSOS__SHARED_H

/////////////////
// Data structures shared between userspace and kernel
/////////////////

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
        // Is process currently blocked (main thread only)
        bool blocked;
        // Filename of process excecutable
        char fileName[32];
    };

    // Contains information about a disk device present on system
    struct DiskInfo
    {
        //Disk identifier string
        char identifier[100];
    };
}
#endif