#include <system/vfs/virtualfilesystem.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

VirtualFileSystem::VirtualFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size, char* name)
{
    this->disk = disk;
    this->SizeInSectors = size;
    this->StartLBA = start;
    this->Name = name;
}

bool VirtualFileSystem::Initialize()
{
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

int VirtualFileSystem::ReadFile(char* path, uint8_t* buffer, uint32_t offset, int size)
{
    return -1;
}

bool VirtualFileSystem::FileExists(char* path)
{
    return false;
}
bool VirtualFileSystem::DirectoryExists(char* path)
{
    return false;
}