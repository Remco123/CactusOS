#include <system/vfs/virtualfilesystem.h>
#include <system/log.h>

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

VirtualFileSystem::~VirtualFileSystem()
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
}

bool VirtualFileSystem::Initialize()
{
    return false;
}

int VirtualFileSystem::ReadFile(const char* filename, uint8_t* buffer, uint32_t offset, uint32_t len)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return -1;
}
int VirtualFileSystem::WriteFile(const char* filename, uint8_t* buffer, uint32_t len, bool create)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return -1;
}
bool VirtualFileSystem::FileExists(const char* filename)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
bool VirtualFileSystem::DirectoryExists(const char* filename)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return false;
}
int VirtualFileSystem::CreateFile(const char* path)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return -1;
}
int VirtualFileSystem::CreateDirectory(const char* path)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return -1;
}
uint32_t VirtualFileSystem::GetFileSize(const char* filename)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return -1;
}
List<LIBCactusOS::VFSEntry>* VirtualFileSystem::DirectoryList(const char* path)
{
    Log(Error, "Virtual function called directly %s:%d", __FILE__, __LINE__);
    return 0;
}