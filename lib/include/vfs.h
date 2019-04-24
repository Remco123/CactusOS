#ifndef __CACTUSOSLIB__VFS_H
#define __CACTUSOSLIB__VFS_H

#include <types.h>

namespace LIBCactusOS
{
    int GetFileSize(char* path);
    int ReadFile(char* path, uint8_t* buffer);
    bool FileExists(char* path);
    bool DirectoryExists(char* path);
}

#endif