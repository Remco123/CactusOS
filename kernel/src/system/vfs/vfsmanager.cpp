#include <system/vfs/vfsmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

VFSManager::VFSManager()
{
    this->Filesystems = new List<VirtualFileSystem*>();
}

int VFSManager::ExtractDiskNumber(char* path, uint8_t* idSizeReturn)
{
    if(String::Contains(path, ':') && String::Contains(path, PATH_SEPERATOR_C))
    {
        int idLength = String::IndexOf(path, ':');

        char* idStr = new char[idLength];
        MemoryOperations::memcpy(idStr, path, idLength);

        int idValue = Convert::StringToInt(idStr);

        delete idStr;

        if(idSizeReturn != 0)
            *idSizeReturn = idLength;   
           
        return idValue;  
    }
    return -1;
}

void VFSManager::Mount(VirtualFileSystem* vfs)
{
    this->Filesystems->push_back(vfs); //Just add it to the list of known filesystems, easy.
}

List<char*>* VFSManager::DirectoryList(char* path)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);
    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->DirectoryList(path + idSize + 2); // skips the 0:\ part
    else
        return 0;
}

int VFSManager::GetFileSize(char* path)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);
    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->GetFileSize(path + idSize + 2); // skips the 0:\ part
    else
        return -1;
}

int VFSManager::ReadFile(char* path, uint8_t* buffer, uint32_t offset, int size)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);

    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->ReadFile(path + idSize + 2, buffer, offset, size);
    else
        return -1;
}

bool VFSManager::FileExists(char* path)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);

    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->FileExists(path + idSize + 2);
    else
        return false;
}

bool VFSManager::DirectoryExists(char* path)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);

    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->DirectoryExists(path + idSize + 2);
    else
        return false;
}