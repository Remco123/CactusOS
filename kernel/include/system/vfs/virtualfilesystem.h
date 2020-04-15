#ifndef __CACTUSOS__SYSTEM__VFS__VIRTUALFILESYSTEM_H
#define __CACTUSOS__SYSTEM__VFS__VIRTUALFILESYSTEM_H

#include <common/types.h>
#include <common/list.h>

#include <system/disks/disk.h>

namespace CactusOS
{
    namespace system
    {
        #define PATH_SEPERATOR_C '\\' //Path Seperator as char
        #define PATH_SEPERATOR_S "\\" //Path Seperator as string

        class VirtualFileSystem
        {
        friend class VFSManager;
        public:
            Disk* disk;
        protected:
            common::uint32_t StartLBA;
            common::uint32_t SizeInSectors;
            
            char* Name = "Unkown";
        public:
            VirtualFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size, char* name = 0);
            virtual bool Initialize();
            
            /////////////
            // VFS Functions (Read, Write, etc.)
            /////////////     
            virtual List<char*>* DirectoryList(char* path);
            virtual int GetFileSize(char* path);
            virtual int ReadFile(char* path, common::uint8_t* buffer);
            virtual bool FileExists(char* path);
            virtual bool DirectoryExists(char* path);
        };
    }
}

#endif