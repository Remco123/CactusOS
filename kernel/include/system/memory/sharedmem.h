#ifndef __CACTUSOS__SYSTEM__MEMORY__SHAREDMEM_H
#define __CACTUSOS__SYSTEM__MEMORY__SHAREDMEM_H

#include <system/tasking/process.h>

namespace CactusOS
{
    namespace system
    {
        class SharedMemory
        {
        public:
            /**
             * Create a shared area of memory between processes
             * Proc1: Procces 1 where the memory needs to be present
             * Proc2: Process 2 where the memory needs to be present
             * virtStart: The virtual start of the memory region
             * len: The length of the memory region
            */
            static bool CreateSharedRegion(Process* proc1, Process* proc2, common::uint32_t virtStart, common::uint32_t len);
            /**
             * Create a shared area of memory between processes
             * Proc1: Procces 1 where the memory needs to be present
             * Proc2: Process 2 where the memory needs to be present
             * virtStart1: The virtual start of the memory region for process 1
             * virtStart2: The virtual start of the memory region for process 2
             * len: The length of the memory region
            */
            static bool CreateSharedRegion(Process* proc1, Process* proc2, common::uint32_t virtStart1, common::uint32_t virtStart2, common::uint32_t len);
            /**
             * Remove shared memory between 2 processes 
            */
            static bool RemoveSharedRegion(Process* proc1, Process* proc2, common::uint32_t virtStart, common::uint32_t len);
            /**
             * Remove shared memory between 2 processes 
            */
            static bool RemoveSharedRegion(Process* proc1, Process* proc2, common::uint32_t virtStart1, common::uint32_t virtStart2, common::uint32_t len);
        };
    }
}

#endif