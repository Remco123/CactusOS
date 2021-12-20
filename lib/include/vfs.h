#ifndef __CACTUSOSLIB__VFS_H
#define __CACTUSOSLIB__VFS_H

#include <types.h>
#include <list.h>
#include <shared.h>

namespace LIBCactusOS
{
    // Read file contents into buffer
    int ReadFile(char* filename, uint8_t* buffer, uint32_t offset = 0, uint32_t len = -1);
    // Write buffer to file, file will be created when create equals true
    int WriteFile(char* filename, uint8_t* buffer, uint32_t len, bool create = true);

    // Check if file exist
    bool FileExists(char* filename);
    // Check if directory exist
    bool DirectoryExists(char* filename);

    // Create a file at the filepath
    int CreateFile(char* path);
    // Create a new directory
    int CreateDirectory(char* path);

    // Get size of specified file in bytes
    uint32_t GetFileSize(char* filename);

    // Get list of files/directories in specified path
    List<VFSEntry> DirectoryListing(char* path);

    // Request to eject a specific disk (only works for CD's at the moment, TODO: usb as well?)
    bool EjectDisk(char* path);
}

#endif