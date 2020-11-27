#ifndef __CACTUSOS__SYSTEM__INITRD_H
#define __CACTUSOS__SYSTEM__INITRD_H

#include <common/types.h>
#include <multiboot/multiboot.h>
#include <system/bootconsole.h>
#include <common/string.h>

namespace CactusOS
{
    namespace system
    {
        struct InitrdFileHeader
        {
            char name[30];
            char path[100];
            common::uint32_t size;
        } __attribute__((packed));

        class InitialRamDisk
        {
        public:
            static void Initialize(multiboot_info_t* mbi);

            static void* ReadFile(const char* path, common::uint32_t* fileSizeReturn = 0);
        };
    }
}

#endif