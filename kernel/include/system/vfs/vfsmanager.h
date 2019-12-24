#ifndef __CACTUSOS__SYSTEM__VFS__VFSMANAGER_H
#define __CACTUSOS__SYSTEM__VFS__VFSMANAGER_H

#include <system/vfs/virtualfilesystem.h>

namespace CactusOS
{
    namespace system
    {
        class VFSManager
        {
        private:
            List<VirtualFileSystem*>* Filesystems;
        public:
            int bootPartitionID = 0;

            VFSManager();
            void Mount(VirtualFileSystem* vfs);
            void Unmount(VirtualFileSystem* vfs);
            void UnmountByDisk(Disk* disk);

            int ExtractDiskNumber(char* path, common::uint8_t* idSizeReturn);
            bool SearchBootPartition();

            /////////////
            // Filesystem functions
            /////////////
            List<char*>* DirectoryList(char* path);
            int GetFileSize(char* path);
            int ReadFile(char* path, common::uint8_t* buffer);
            bool FileExists(char* path);
            bool DirectoryExists(char* path);
            bool EjectDrive(char* path);
        };
    }
}

#endif