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
int LIBCactusOS::ReadFile(char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{
    return (int)DoSyscall(SYSCALL_READ_FILE, (uint32_t)path, (uint32_t)buffer, offset, len);
}
int LIBCactusOS::WriteFile(char* path, uint8_t* buffer, uint32_t len, bool create)
{
    return (int)DoSyscall(SYSCALL_WRITE_FILE, (uint32_t)path, (uint32_t)buffer, len, (uint32_t)create);
}
int LIBCactusOS::CreateFile(char* path)
{
    return (int)DoSyscall(SYSCALL_CREATE_FILE, (uint32_t)path);
}
int LIBCactusOS::CreateDirectory(char* path)
{
    return (int)DoSyscall(SYSCALL_CREATE_DIRECTORY, (uint32_t)path);
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
    if(items == -1)
        return result;
    
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
List<DiskInfo> LIBCactusOS::DiskListing()
{
    List<DiskInfo> result;

    int items = DoSyscall(SYSCALL_BEGIN_LISTING, DISK_LISTING);
    if(items == -1)
        return result;
    
    for(int i = 0; i < items; i++) {
        DiskInfo buf;
        int succes = DoSyscall(SYSCALL_LISTING_ENTRY, DISK_LISTING, (uint32_t)i, (uint32_t)&buf);
        if(succes == 0) {
            return result; //End of items
        }

        result += buf;
    }
    DoSyscall(SYSCALL_END_LISTING, DISK_LISTING);
    return result;
}