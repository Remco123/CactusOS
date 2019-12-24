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

        int idValue = 0;

        if(isalpha(idStr[0])) //Are we using a character instead of a integer
        {
            switch (idStr[0])
            {
                case 'b':
                case 'B': //Boot partition
                    idValue = this->bootPartitionID;
                    break;      
                default:
                    delete idStr;
                    return -1;
                    break;
            }
        }
        else
            idValue = Convert::StringToInt(idStr);

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
void VFSManager::Unmount(VirtualFileSystem* vfs)
{
    this->Filesystems->Remove(vfs);
}
void VFSManager::UnmountByDisk(Disk* disk)
{
    for(VirtualFileSystem* vfs : *Filesystems)
        if(vfs->disk == disk)
            Unmount(vfs);
}

bool VFSManager::SearchBootPartition()
{
    char* pathString = "####:\\boot\\CactusOS.bin";
    for(int i = 0; i < Filesystems->size(); i++)
    {
        char* idStr = Convert::IntToString(i);
        int idStrLen = String::strlen(idStr);

        MemoryOperations::memcpy(pathString + (4-idStrLen), idStr, idStrLen);

        if(Filesystems->GetAt(i)->FileExists(pathString + (4-idStrLen) + 3))
        {
            this->bootPartitionID = i;
            return true;
        }
    }
    return false;
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

int VFSManager::ReadFile(char* path, uint8_t* buffer)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);

    if(disk != -1 && Filesystems->size() > disk)
        return Filesystems->GetAt(disk)->ReadFile(path + idSize + 2, buffer);
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

    if(disk != -1 && Filesystems->size() > disk) {
        if(String::strlen(path) == idSize + 2) //Only disk part, like 0:\ ofcourse this is a directory as well
            return true;
        else
            return Filesystems->GetAt(disk)->DirectoryExists(path + idSize + 2);
    }
    else
        return false;
}

bool VFSManager::EjectDrive(char* path)
{
    uint8_t idSize = 0;
    int disk = ExtractDiskNumber(path, &idSize);

    if(disk != -1 && Filesystems->size() > disk) {
        VirtualFileSystem* fs = Filesystems->GetAt(disk);
        return fs->disk->controller->EjectDrive(fs->disk->controllerIndex);
    }
    else
        return false;
}