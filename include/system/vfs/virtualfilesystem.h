#ifndef __CACTUSOS__SYSTEM__VIRTUALFILESYSTEM_H
#define __CACTUSOS__SYSTEM__VIRTUALFILESYSTEM_H

#include <common/types.h>

#include <system/disks/disk.h>

namespace CactusOS
{
    namespace system
    {
        class VirtualFileSystem
        {
        protected:
            Disk* disk;
            common::uint32_t StartLBA;
            common::uint32_t SizeInSectors;
            
            bool ReadOnly = false;
        public:
            VirtualFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size);

            virtual bool Initialize();
        };
    }
}

#endif