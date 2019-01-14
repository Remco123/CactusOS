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
            VFSManager();
            void Mount(VirtualFileSystem* vfs);

            int ExtractDiskNumber(char* path, common::uint8_t* idSizeReturn);

            /////////////
            // Filesystem functions
            /////////////
            List<char*>* DirectoryList(char* path);
            int GetFileSize(char* path);
            int ReadFile(char* path, common::uint8_t* buffer, common::uint32_t offset = 0, int size = -1);
        };
    }
}

#endif