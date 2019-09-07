#include <vfs.h>

#include <syscall.h>
#include <listing.h>

using namespace LIBCactusOS;

bool LIBCactusOS::FileExists(char* path)
{
    return (bool)DoSyscall(SYSCALL_FILE_EXISTS, (uint32_t)path);
}
uint32_t LIBCactusOS::GetFileSize(char* path)
{
    return (uint32_t)DoSyscall(SYSCALL_GET_FILESIZE, (uint32_t)path);
}
int LIBCactusOS::ReadFile(char* path, uint8_t* buffer)
{
    return (int)DoSyscall(SYSCALL_READ_FILE, (uint32_t)path, (uint32_t)buffer);
}
bool LIBCactusOS::DirectoryExists(char* path)
{
    return (bool)DoSyscall(SYSCALL_DIR_EXISTS, (uint32_t)path);
}
bool LIBCactusOS::EjectDisk(char* path)
{
    return (bool)DoSyscall(SYSCALL_EJECT_DISK, (uint32_t)path);
}
List<char*> LIBCactusOS::DirectoryListing(char* path)
{
    List<char*> result;

    int items = DoSyscall(SYSCALL_BEGIN_LISTING, DIRECTORY_LISTING, (uint32_t)path);
    for(int i = 0; i < items; i++) {
        char* strBuf = new char[100];
        int strLen = DoSyscall(SYSCALL_LISTING_ENTRY, DIRECTORY_LISTING, (uint32_t)i, (uint32_t)strBuf);
        if(strLen == 0) {
            delete strBuf;
            return result; //End of items
        }

        result += strBuf;
    }

    DoSyscall(SYSCALL_END_LISTING, DIRECTORY_LISTING);
    return result;
}