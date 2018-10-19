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


            int ExtractDiskNumber(char* path);
        public:
            VFSManager();

            void Mount(VirtualFileSystem* vfs);


            /////////////
            // Filesystem functions
            /////////////
            List<char*>* DirectoryList(char* path);
            
            int GetFileSize(char* path);

            int ReadFile(char* path, common::uint8_t* buffer);
        };
    }
}

#endif