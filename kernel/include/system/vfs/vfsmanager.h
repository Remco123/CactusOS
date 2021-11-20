#ifndef __CACTUSOS__SYSTEM__VFS__VFSMANAGER_H
#define __CACTUSOS__SYSTEM__VFS__VFSMANAGER_H

#include <system/vfs/virtualfilesystem.h>

namespace CactusOS
{
    namespace system
    {
        class VFSManager
        {
        public:
            List<VirtualFileSystem*>* Filesystems;
        public:
            int bootPartitionID = -1;

            VFSManager();
            void Mount(VirtualFileSystem* vfs);
            void Unmount(VirtualFileSystem* vfs);
            void UnmountByDisk(Disk* disk);

            int ExtractDiskNumber(const char* path, common::uint8_t* idSizeReturn);
            bool SearchBootPartition();

            /////////////
            // Filesystem functions
            /////////////

            // Read file contents into buffer
            int ReadFile(const char* filename, uint8_t* buffer, uint32_t offset = 0, uint32_t len = -1);
            // Write buffer to file, file will be created when create equals true
            int WriteFile(const char* filename, uint8_t* buffer, uint32_t len, bool create = true);

            // Check if file exist
            bool FileExists(const char* filename);
            // Check if directory exist
            bool DirectoryExists(const char* filename);

            // Create a file at the filepath
            int CreateFile(const char* path);
            // Create a new directory
            int CreateDirectory(const char* path);

            // Get size of specified file in bytes
            uint32_t GetFileSize(const char* filename);
            // Returns list of context inside a directory
            List<LIBCactusOS::VFSEntry>* DirectoryList(const char* path);

            ///////////////////
            // Higher Level Functions
            ///////////////////

            // Eject the drive given by a path
            bool EjectDrive(const char* path);
        };
    }
}

#endif