#ifndef __CACTUSOS__SYSTEM__VFS__VIRTUALFILESYSTEM_H
#define __CACTUSOS__SYSTEM__VFS__VIRTUALFILESYSTEM_H

#include <common/types.h>
#include <common/list.h>

#include <system/disks/disk.h>
#include <../../lib/include/shared.h>

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
            virtual ~VirtualFileSystem();
            
            virtual bool Initialize();
            
            /////////////
            // VFS Functions (Read, Write, etc.)
            ///////////// 
               
            // Read file contents into buffer
            virtual int ReadFile(const char* filename, uint8_t* buffer, uint32_t offset = 0, uint32_t len = -1);
            // Write buffer to file, file will be created when create equals true
            virtual int WriteFile(const char* filename, uint8_t* buffer, uint32_t len, bool create = true);

            // Check if file exist
            virtual bool FileExists(const char* filename);
            // Check if directory exist
            virtual bool DirectoryExists(const char* filename);

            // Create a file at the filepath
            virtual int CreateFile(const char* path);
            // Create a new directory
            virtual int CreateDirectory(const char* path);

            // Get size of specified file in bytes
            virtual uint32_t GetFileSize(const char* filename);
            // Returns list of context inside a directory
            virtual List<LIBCactusOS::VFSEntry>* DirectoryList(const char* path);
        };
    }
}

#endif