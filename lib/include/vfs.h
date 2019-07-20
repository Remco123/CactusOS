#ifndef __CACTUSOSLIB__VFS_H
#define __CACTUSOSLIB__VFS_H

#include <types.h>

namespace LIBCactusOS
{
    uint32_t GetFileSize(char* path);
    int ReadFile(char* path, uint8_t* buffer);
    bool FileExists(char* path);
    bool DirectoryExists(char* path);
    bool EjectDisk(char* path);
}

#endif