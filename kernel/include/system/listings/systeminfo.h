#ifndef __CACTUSOS__SYSTEM__LISTINGS__SYSTEMINFO_H
#define __CACTUSOS__SYSTEM__LISTINGS__SYSTEMINFO_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        struct SIBIOS
        {
            char* vendor = "N/A";
            char* version = "N/A";
            char* releaseDate = "N/A";
        };

        struct SISYSTEM
        {
            char* manufacturer = "N/A";
            char* product = "N/A";
            char* version = "N/A";
            char* serial = "N/A";
            char* sku = "N/A";
            char* family = "N/A";  
        };

        struct SIENCLOSURE
        {
            char* manufacturer = "N/A";
            char* version = "N/A";
            char* serial = "N/A";
            char* sku = "N/A";
        };

        struct SIPROCESSOR
        {
            char* socket = "N/A";
            char* manufacturer = "N/A";
            char* version = "N/A";
        };       

        class SystemInfoManager
        {
        public:
            static SIBIOS       bios;
            static SISYSTEM     system;
            static SIENCLOSURE  enclosure;
            static SIPROCESSOR  processor;
        public:
            // Handle a request from a syscall to get information about the system
            static bool HandleSysinfoRequest(void* arrayPointer, int count, common::uint32_t retAddr, bool getSize);
        };
    }
}

#endif