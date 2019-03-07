#include <vfs.h>

#include <syscall.h>

bool LIBCactusOS::FileExists(char* path)
{
    return (bool)DoSyscall(SYSCALL_FILE_EXISTS, (uint32_t)path);
}
int LIBCactusOS::GetFileSize(char* path)
{
    return (int)DoSyscall(SYSCALL_GET_FILESIZE, (uint32_t)path);
}
int LIBCactusOS::ReadFile(char* path, uint8_t* buffer)
{
    return (int)DoSyscall(SYSCALL_READ_FILE, (uint32_t)path, (uint32_t)buffer);
}
bool LIBCactusOS::DirectoryExists(char* path)
{
    return (bool)DoSyscall(SYSCALL_DIR_EXISTS, (uint32_t)path);
}