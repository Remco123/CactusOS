#include <system/vfs/iso9660.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);
void printbits(uint8_t);

void printlen(char* str, int len)
{
    char* buf = new char[len];
    MemoryOperations::memcpy(buf, str, len);
    buf[len] = '\0';
    printf(buf);
    delete buf;
}

//with this we don't need to allocate and free buffers all the time
uint8_t readBuffer[CDROM_SECTOR_SIZE];

ISO9660::ISO9660(Disk* disk, common::uint32_t start, common::uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->ReadOnly = true;
    this->FilesystemName = "ISO9660 Filesystem";
}

bool ISO9660::Initialize()
{
    printf("Starting ISO9660 fileystem\n");
    printf("Finding Volume Descriptors\n");
    bool FoundPVD = false;
    int Offset = ISO_START_SECTOR;

    while (FoundPVD == false)
    {
        this->disk->ReadSector(Offset, readBuffer);
        VolumeDescriptor* descriptor = (VolumeDescriptor*)readBuffer;
        printf("Found, type = "); printf(Convert::IntToString(descriptor->Type)); printf("\n");

        if(descriptor->Type == VolumeDescriptorType::PVDescriptor)
        {
            FoundPVD = true;
            printf("Found Primary Volume Descriptor at offset: "); printf(Convert::IntToString(Offset)); printf("\n");

            //Print PrimaryVolumeDescriptor information
            PrimaryVolumeDescriptor* pvd = (PrimaryVolumeDescriptor*)readBuffer;
            printf("Identifier: "); printf(pvd->id); printf("\n");
            printf("Volume Identifier: "); printf(pvd->volume_id); printf("\n");
            printf("Preparer: "); printlen(pvd->preparer_id, 80); printf("\n");
            printf("Version: "); printf(Convert::IntToString((uint8_t)pvd->version)); printf("\n");
            printf("Root directory sector: "); printf(Convert::IntToString(pvd->root_directory_record.extent_location)); printf("\n");
            printf("Creation date: "); printlen(pvd->creation_date.Year, 5); printf("-"); printlen(pvd->creation_date.Month + 1, 2); printf("-"); printlen(pvd->creation_date.Day + 1, 2); printf(" : "); printlen(pvd->creation_date.Hour + 1, 2); printf(":"); printlen(pvd->creation_date.Minute + 1, 2); printf(":"); printlen(pvd->creation_date.Second + 1, 2); printf("\n");
        
            printf("Saving root directory to memory\n");
            this->disk->ReadSector(pvd->root_directory_record.extent_location, readBuffer);
            this->rootDirectory = (DirectoryRecord*)MemoryManager::activeMemoryManager->malloc(sizeof(DirectoryRecord));
            MemoryOperations::memcpy(this->rootDirectory, readBuffer, sizeof(DirectoryRecord));
        }
        else if (descriptor->Type == VolumeDescriptorType::VolumeDescriptorSetTerminator)
        {
            printf("Found end of descriptors\n");
            Offset = this->SizeInSectors + (uint32_t)ISO_START_SECTOR + 1234; //We reached the end of the descriptors
        }                                                                     //                               |
                                                                              //                               |
        if(Offset < (this->SizeInSectors - (uint32_t)ISO_START_SECTOR))       //                               |
            Offset++; //Read the next sector                                  //                               |
        else // <----------------------------------------------------------------------------------------------- So this gets called 
        {
            printf("Could not find valid Primary Volume Descriptor\n");
            return false;
        }
    }

    printf("Root dir length: "); printf(Convert::IntToString(rootDirectory->length)); printf("\n");
    printf("Flags: 0b"); printbits(rootDirectory->flags); printf("\n");
}

DirectoryRecord* ISO9660::SearchForEntry(DirectoryRecord* searchIn, char* name)
{
    int Offset = ((searchIn == rootDirectory) ? searchIn->length : 0);
    int SectorOffset = 1;

    this->disk->ReadSector(searchIn->extent_location, readBuffer);

    while(true)
    {
        if(Offset > CDROM_SECTOR_SIZE) //We reached the end of the sector, so we want to read the next one
        {
            //printf("Reading new sector, we reached the end\n");
            Offset = 0; //Reset the offset in the sector
            this->disk->ReadSector(searchIn->extent_location + SectorOffset, readBuffer);
            SectorOffset++;
        }

        DirectoryRecord* record = (DirectoryRecord*)(readBuffer + Offset);
        if(record->length == 0) //We reached the end of the entry's
        {
            break;
        }
        else
        {
            //printf("Found entry: "); printlen(record->name, record->name_length);
            if(String::strcmp(name, record->name)) //We found the correct entry!
            {
                //printf(" [Correct]\n");
                //Allocate the return result, we do this because the readbuffer gets deleted so it can be overwritten
                DirectoryRecord* returnResult = (DirectoryRecord*)MemoryManager::activeMemoryManager->malloc(sizeof(DirectoryRecord) + record->name_length);
                MemoryOperations::memcpy(returnResult, record, sizeof(DirectoryRecord) + record->name_length);

                return returnResult;
            }
            //printf("\n");
        }

        Offset += record->length;
    }

    return 0;
}

DirectoryRecord* ISO9660::GetEntry(char* path)
{
    DirectoryRecord* cur = this->rootDirectory;

    char** pathArray;
    int dirCount = String::Split(path, '\\', &pathArray);
    //printf("Dir count: "); printf(Convert::IntToString(dirCount)); printf("\n");
    if(dirCount == 1 && (String::strlen(path) == 1)) //the path is the root directory
    {
        return cur;
    }
    for(int i = 0; i < dirCount; i++)
    {
        //printf("Searching for "); printf(pathArray[i]); printf(" in "); printf(i > 0 ? pathArray[i - 1] : (char*)"root"); printf("\n");
        DirectoryRecord* entry = this->SearchForEntry(cur, pathArray[i]);
        if(entry == 0)
        {
            //printf("Could not found entry: "); printf(pathArray[i]); printf("\n");
            return 0;
        }
        else
        {
            if(String::strcmp(entry->name, pathArray[i]) && i == dirCount - 1)
            {
                return entry;
            }
        }

        cur = entry;
    }
    return 0;
}

Iso_EntryType ISO9660::GetEntryType(DirectoryRecord* entry)
{
    return ((entry->flags >> 1) & 1) ? Iso_Directory : Iso_File;
}





List<char*>* ISO9660::DirectoryList(char* path)
{
    List<char*>* result = new List<char*>();
    DirectoryRecord* parent = String::strlen(path) > 0 ? GetEntry(path) : rootDirectory;
    if(GetEntryType(parent) == Iso_File)
    {
        result->push_back("[Error]");
        return result;
    }


    int Offset = ((parent == rootDirectory) ? parent->length : 0);
    int SectorOffset = 1;
    this->disk->ReadSector(parent->extent_location, readBuffer);

    while(true)
    {
        if(Offset > CDROM_SECTOR_SIZE) //We reached the end of the sector, so we want to read the next one
        {
            Offset = 0; //Reset the offset in the sector
            this->disk->ReadSector(parent->extent_location + SectorOffset, readBuffer);
            SectorOffset++;
        }

        DirectoryRecord* record = (DirectoryRecord*)(readBuffer + Offset);
        if(record->length == 0) //We reached the end of the entry's
        {
            break;
        }
        else
        {
            if(record->name[0] != '\0' && record->name[0] != '\1') //We ignore the . and .. directories
            {
                char* entry = (char*)MemoryManager::activeMemoryManager->malloc(record->name_length + 1);
                MemoryOperations::memcpy(entry, record->name, record->name_length);
                entry[record->name_length] = '\0';
                result->push_back(entry);
            }
        }

        Offset += record->length;
    }

    return result;
}

int ISO9660::GetFileSize(char* path)
{
    DirectoryRecord* entry = String::strlen(path) > 0 ? GetEntry(path) : rootDirectory;
    if(entry == 0 || GetEntryType(entry) == Iso_Directory)
    {
        printf("File ("); printf(path); printf(") not found or not a file\n");
        return -1;
    }

    return entry->data_length;
}