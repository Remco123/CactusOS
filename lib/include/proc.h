#ifndef __LIBCACTUSOS__PROC_H
#define __LIBCACTUSOS__PROC_H

#include <syscall.h>
#include <types.h>
#include <systeminfo.h>
#include <list.h>
#include <shared.h>

namespace LIBCactusOS
{
    #define DECLARE_LOCK(name) volatile int name ## Locked
    #define LOCK(name) \
	    while (name ## Locked == 1) asm("pause"); \
	    __sync_synchronize();
    #define UNLOCK(name) \
	    __sync_synchronize(); \
	    name ## Locked = 0;

    class Process
    {
    private:
        /**
         * How many threads does this application have?
        */
        static int numThreads;
    public:
        /**
         * The PID of this process
        */
        static int ID;
        /**
         * A pointer to a buffer shared with the kernel that contains some systeminfo
        */
        static SharedSystemInfo* systemInfo;
        /**
         * Start a new process and return its PID, if block is set to true the process will not be started automaticly.
        */
        static int Run(const char* path, bool block = false);
        /**
         * Create a block of shared memory
        */
        static bool CreateSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len);
        /**
         * Create a block of shared memory
        */
        static bool CreateSharedMemory(int proc2ID, uint32_t virtStart1, uint32_t virtStart2, uint32_t len);
        /**
         * Remove a block of shared memory 
        */
        static bool DeleteSharedMemory(int proc2ID, uint32_t virtStart, uint32_t len);
        /**
         * Remove a block of shared memory
        */
        static bool DeleteSharedMemory(int proc2ID, uint32_t virtStart1, uint32_t virtStart2, uint32_t len);
        /**
         * Create a new thread for this application
        */
        static void CreateThread(void (*entryPoint)(), bool switchTo = false);
        /**
         * Force a task switch
        */
        static void Yield();
        /**
         * Write a byte to this processes standard output stream 
        */
        static void WriteStdOut(char byte);
        /**
         * Write bytes to this processes standard output stream 
        */
        static void WriteStdOut(char* bytes, int length);
        /**
         * Read a byte from this processes standard input stream
        */
        static char ReadStdIn();
        /**
         * How many bytes can be read from the stdin stream? 
        */
        static int StdInAvailable();
        /**
         * Redirect one process output to another process input 
        */
        static void BindSTDIO(int fromID, int toID);
        /**
         * Returns if a process with given pid is present and running.
        */
        static bool Active(int pid);
        /**
         * Unblock a thread of a process
        */
        static void Unblock(int procPID, int thread = 0);
        /**
         * Dissable or enable kernel scheduler
        */
        static void SetScheduler(bool active);
    };
}

#endif