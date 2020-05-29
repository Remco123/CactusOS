#include <system/vfs/fat.h>

#include <common/print.h>
#include <system/log.h>
#include <system/system.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

FAT::FAT(Disk* disk, uint32_t start, uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->Name = "FAT Filesystem";
}

bool FAT::Initialize()
{
    BootConsole::WriteLine();
    BootConsole::WriteLine("Initializing FAT Filesystem");
    
    return false;
}

List<char*>* FAT::DirectoryList(const char* path)
{ 
    List<char*>* ret = new List<char*>();    
    return ret;
}

uint32_t FAT::GetFileSize(const char* path)
{
    return -1;
}
int FAT::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{ 
    if(len == -1)
        len = GetFileSize(path);

    return 0;
}

int FAT::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create)
{

    return 0;
}

int FAT::CreateFile(const char* path)
{

    return 0;
}

int FAT::CreateDirectory(const char* path)
{
    return 0;
}

bool FAT::FileExists(const char* path)
{

    return false;
}
bool FAT::DirectoryExists(const char* path)
{

    return false;
}