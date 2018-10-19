#include <system/vfs/virtualfilesystem.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

VirtualFileSystem::VirtualFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size)
{
    this->disk = disk;
    this->SizeInSectors = size;
    this->StartLBA = start;
}

bool VirtualFileSystem::Initialize()
{
    printf("Can not initialize filesystem that is zero!\n");
    return false;
}

List<char*>* VirtualFileSystem::DirectoryList(char* path)
{
    return 0;
}

int VirtualFileSystem::GetFileSize(char* path)
{
    return -1;
}

int VirtualFileSystem::ReadFile(char* path, uint8_t* buffer)
{
    return -1;
}