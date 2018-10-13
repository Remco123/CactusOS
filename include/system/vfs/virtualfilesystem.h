#ifndef __CACTUSOS__SYSTEM__VIRTUALFILESYSTEM_H
#define __CACTUSOS__SYSTEM__VIRTUALFILESYSTEM_H

#include <common/types.h>
#include <common/list.h>

#include <system/disks/disk.h>

namespace CactusOS
{
    namespace system
    {
        #define ROOT_DIR_STR "\\"

        class VirtualFileSystem
        {
        protected:
            Disk* disk;
            common::uint32_t StartLBA;
            common::uint32_t SizeInSectors;
            
            bool ReadOnly = false;
            char* FilesystemName = "Unkown";
        public:
            VirtualFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size);

            virtual bool Initialize();
            
            /////////////
            // VFS Functions (Read, Write, etc.)
            /////////////
            
            virtual List<char*>* DirectoryList(char* path);
        };
    }
}

#endif