#ifndef __CACTUSOS__SYSTEM__LISTINGS__SYSTEMINFO_H
#define __CACTUSOS__SYSTEM__LISTINGS__SYSTEMINFO_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class SystemInfoManager
        {
        public:
            // Handle a request from a syscall to get information about the system
            static bool HandleSysinfoRequest(void* arrayPointer, int count, common::uint32_t retAddr, bool getSize);
        };
    }
}

#endif