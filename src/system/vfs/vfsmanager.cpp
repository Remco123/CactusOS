#include <system/vfs/vfsmanager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

VFSManager::VFSManager()
{
    this->Filesystems = new List<VirtualFileSystem*>();
}

int VFSManager::ExtractDiskNumber(char* path)
{
    if(path[1] == ':' && path[2] == '\\')
    {
        return Convert::StringToInt(Convert::CharToString(path[0]));
    }
    else
        return -1;
}

void VFSManager::Mount(VirtualFileSystem* vfs)
{
    this->Filesystems->push_back(vfs); //Just add it to the list of known filesystems, easy.
    printf("Mounted "); printf(vfs->FilesystemName); printf(" at "); printf(Convert::IntToString(this->Filesystems->size() - 1)); printf("\n");
}

List<char*>* VFSManager::DirectoryList(char* path)
{
    int disk = ExtractDiskNumber(path);
    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->DirectoryList(path + 3); // + 3 skips the 0:\ part
    else
        return 0;
}

int VFSManager::GetFileSize(char* path)
{
    int disk = ExtractDiskNumber(path);
    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->GetFileSize(path + 3);
    else
        return -1;
}

int VFSManager::ReadFile(char* path, uint8_t* buffer)
{
    int disk = ExtractDiskNumber(path);
    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->ReadFile(path + 3, buffer);
    else
        return -1;
}