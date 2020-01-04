#ifndef __CACTUSOSLIB__VFS_H
#define __CACTUSOSLIB__VFS_H

#include <types.h>
#include <list.h>
#include <shared.h>

namespace LIBCactusOS
{
    uint32_t GetFileSize(char* path);
    int ReadFile(char* path, uint8_t* buffer);
    bool FileExists(char* path);
    bool DirectoryExists(char* path);
    bool EjectDisk(char* path);
    List<char*> DirectoryListing(char* path);
    List<DiskInfo> DiskListing();
}

#endif