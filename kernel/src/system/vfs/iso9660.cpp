//TODO: There is probably a bit of a memory leak here, fix this in the future

#include <system/vfs/iso9660.h>

#include <system/memory/heap.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

//with this we don't need to allocate and free buffers all the time
uint8_t readBuffer[CDROM_SECTOR_SIZE];

ISO9660::ISO9660(Disk* disk, uint32_t start, uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->Name = "ISO9660 Filesystem";
}

bool ISO9660::Initialize()
{
    BootConsole::WriteLine();
    BootConsole::WriteLine("Starting ISO9660 fileystem");
    BootConsole::WriteLine("Reading Volume Descriptors");
    bool FoundPVD = false;
    int Offset = ISO_START_SECTOR;

    while (FoundPVD == false)
    {
        if(this->disk->ReadSector(Offset, readBuffer) != 0)
        {
            BootConsole::WriteLine("Error Reading disk");
            return false;
        }

        VolumeDescriptor* descriptor = (VolumeDescriptor*)readBuffer;
        BootConsole::Write("Found, type = "); BootConsole::Write(Convert::IntToString(descriptor->Type)); BootConsole::WriteLine();

        if(descriptor->Type == VolumeDescriptorType::PVDescriptor)
        {
            FoundPVD = true;
            BootConsole::Write("Found Primary Volume Descriptor at offset: "); BootConsole::Write(Convert::IntToString(Offset)); BootConsole::WriteLine();

            //Print PrimaryVolumeDescriptor information
            PrimaryVolumeDescriptor* pvd = (PrimaryVolumeDescriptor*)readBuffer;
            BootConsole::Write("Identifier: "); BootConsole::Write(pvd->id); BootConsole::WriteLine();
            BootConsole::Write("Volume Identifier: "); BootConsole::Write(pvd->volume_id); BootConsole::WriteLine();
            BootConsole::Write("Version: "); BootConsole::Write(Convert::IntToString(pvd->version)); BootConsole::WriteLine();
            BootConsole::Write("Root directory sector: "); BootConsole::Write(Convert::IntToString(pvd->root_directory_record.extent_location)); BootConsole::WriteLine();
        
            BootConsole::WriteLine("Saving root directory to memory");
            if(this->disk->ReadSector(pvd->root_directory_record.extent_location, readBuffer) != 0)
            {
                BootConsole::WriteLine("Error Reading disk");
                return false;
            }
            this->rootDirectory = (DirectoryRecord*)KernelHeap::malloc(sizeof(DirectoryRecord));
            MemoryOperations::memcpy(this->rootDirectory, readBuffer, sizeof(DirectoryRecord));
        }
        else if (descriptor->Type == VolumeDescriptorType::VolumeDescriptorSetTerminator)
        {
            BootConsole::WriteLine("Found end of descriptors");
            return false;
        }

        if(Offset < (int)(this->SizeInSectors - (uint32_t)ISO_START_SECTOR))
            Offset++; //Read the next sector
        else
        {
            BootConsole::WriteLine("Could not find valid Primary Volume Descriptor");
            return false;
        }
    }

    uint8_t oldColor = BootConsole::ForegroundColor;
    BootConsole::ForegroundColor = VGA_COLOR_GREEN;
    BootConsole::Write("ISO9660 Filesystem Intialized");
    BootConsole::ForegroundColor = oldColor;
    return true;
}

Iso_EntryType ISO9660::GetEntryType(DirectoryRecord* entry)
{
    return ((entry->flags >> 1) & 1) ? Iso_Directory : Iso_File;
}

DirectoryRecord* ISO9660::SearchInDirectory(DirectoryRecord* searchIn, const char* name)
{
    int Offset = ((searchIn == rootDirectory) ? searchIn->length : 0);
    int SectorOffset = 1;

    if(this->disk->ReadSector(searchIn->extent_location, readBuffer) != 0)
        return 0;

    while(true)
    {
        if(Offset + sizeof(DirectoryRecord) > CDROM_SECTOR_SIZE) //We reached the end of the sector, so we need to read the next one
        {
            Offset = 0; //Reset the offset in the sector
            if(this->disk->ReadSector(searchIn->extent_location + SectorOffset, readBuffer) != 0)
                return 0;
            SectorOffset++;
        }

        DirectoryRecord* record = (DirectoryRecord*)(readBuffer + Offset);
        if(record->length == 0) //We reached the end of the entry's
        {
            break;
        }
        else
        {
            char nameBuffer[222];
            int nameLength = record->name_length;

            MemoryOperations::memcpy(nameBuffer, record->name, nameLength);

            if(GetEntryType(record) == Iso_File)
                nameBuffer[nameLength - 2] = '\0'; //Ignore Version
            else
                nameBuffer[nameLength] = '\0';

            if(String::strcmp(nameBuffer, name))
            {
                DirectoryRecord* result = (DirectoryRecord*)KernelHeap::malloc(sizeof(DirectoryRecord) + nameLength + 1);
                MemoryOperations::memcpy(result, record, sizeof(DirectoryRecord) + nameLength);

                //Terminate string
                result->name[nameLength] = '\0';
                return result;
            }
        }

        Offset += record->length;
    }

    return 0;
}

DirectoryRecord* ISO9660::GetEntry(const char* path)
{
    DirectoryRecord* searchIn = this->rootDirectory;

    List<char*> pathList = String::Split(path, PATH_SEPERATOR_C);

    if(pathList.size() == 0) //The path represents a entry in the root directory
        return this->SearchInDirectory(searchIn, path);

    for(int i = 0; i < pathList.size(); i++)
    {
        DirectoryRecord* entry = this->SearchInDirectory(searchIn, pathList[i]);
        if(entry == 0)
            return 0;
        else if(i == pathList.size() - 1)
        {
            return entry;
        }

        searchIn = entry;
    }

    return 0;
}

List<char*>* ISO9660::DirectoryList(const char* path)
{
    List<char*>* result = new List<char*>();
    DirectoryRecord* parent = String::strlen(path) > 0 ? GetEntry(path) : rootDirectory;
    if(parent == 0 || GetEntryType(parent) == Iso_File)
    {
        result->push_back("[Error]");
        return result;
    }

    int Offset = ((parent == rootDirectory) ? parent->length : 0);
    int SectorOffset = 1;
    if(this->disk->ReadSector(parent->extent_location, readBuffer) != 0)
        return result;

    while(true)
    {
        if(Offset + sizeof(DirectoryRecord) > CDROM_SECTOR_SIZE) //We reached the end of the sector, so we want to read the next one, TODO: fix this
        {
            Offset = 0; //Reset the offset in the sector
            if(this->disk->ReadSector(parent->extent_location + SectorOffset, readBuffer) != 0)
                return result;
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
                if(GetEntryType(record) == Iso_File)
                {
                    char* entry = (char*)KernelHeap::malloc(record->name_length);
                    MemoryOperations::memcpy(entry, record->name, record->name_length - 2);
                    entry[record->name_length - 2] = '\0';
                    result->push_back(entry);
                }
                else //Directories do not have version numbers
                {
                    char* entry = (char*)KernelHeap::malloc(record->name_length + 1);
                    MemoryOperations::memcpy(entry, record->name, record->name_length);
                    entry[record->name_length] = '\0';
                    result->push_back(entry);
                }
            }
        }

        Offset += record->length;
    }

    return result;
}
uint32_t ISO9660::GetFileSize(const char* path)
{
    DirectoryRecord* entry = GetEntry(path);
    if(entry == 0 || GetEntryType(entry) == Iso_Directory)
    {
        return -1;
    }

    return entry->data_length;
}
int ISO9660::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{
    DirectoryRecord* entry = GetEntry(path);

    if(entry == 0 || GetEntryType(entry) == Iso_Directory)
        return -1;

    if(len == (uint32_t)-1)
        len = entry->data_length;
    
    /*
    if(len > entry->data_length || (offset + len) > entry->data_length) // File is not this big
        return -1;

    uint8_t* bufPtr = buffer;
    uint32_t startSector = offset / CDROM_SECTOR_SIZE;
    uint32_t startOffset = offset % CDROM_SECTOR_SIZE;
    uint32_t endSector = (offset + len) / CDROM_SECTOR_SIZE;

    if(startOffset != 0) { // There is a partial sector at the beginning that needs to be read
        if(disk->ReadSector(entry->extent_location + startSector, readBuffer) != 0)
            return -1;
        
        // Copy partial data from readBuffer into target buffer
        uint32_t len = CDROM_SECTOR_SIZE - (CDROM_SECTOR_SIZE - startOffset);
        MemoryOperations::memcpy(bufPtr, readBuffer + (CDROM_SECTOR_SIZE - startOffset), len);

        bufPtr += len;
        startSector++;
    }

    for(; startSector < endSector; startSector++) { // Read whole sectors containing file data
        if(disk->ReadSector(entry->extent_location + startSector, bufPtr) != 0)
            return -1;
        
        bufPtr += CDROM_SECTOR_SIZE;
    }

    uint32_t endOffset = (offset + len) % CDROM_SECTOR_SIZE;
    if(endOffset != 0) { // There is a partial sector at the end that needs to be read
        if(disk->ReadSector(entry->extent_location + startSector, readBuffer) != 0)
            return -1;

        MemoryOperations::memcpy(bufPtr, readBuffer, endOffset);
    }
    */

    //TODO: Actually implement partial file reading, for now we read the whole file
    int sectorCount = entry->data_length / CDROM_SECTOR_SIZE;
    int dataRemainder = entry->data_length % CDROM_SECTOR_SIZE;

    for(int i = 0; i < sectorCount; i++)
        if(this->disk->ReadSector(entry->extent_location + i, buffer + (CDROM_SECTOR_SIZE * i)) != 0)
            return -1;
    
    if(dataRemainder > 0) //We have a remainder
    {
        if(this->disk->ReadSector(entry->extent_location + sectorCount, readBuffer) != 0)
            return -1;
        MemoryOperations::memcpy(buffer + (sectorCount*CDROM_SECTOR_SIZE), readBuffer, dataRemainder);
    }
    
    return 0;
}
int ISO9660::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create)
{
    return -1; // ISO9660 is readonly
}
int ISO9660::CreateFile(const char* path)
{
    return -1;
}
int ISO9660::CreateDirectory(const char* path)
{
    return -1;
}

bool ISO9660::FileExists(const char* path)
{
    DirectoryRecord* entry = GetEntry(path);
    if(entry == 0 || GetEntryType(entry) == Iso_Directory)
    {
        return false;
    }

    return true;
}

bool ISO9660::DirectoryExists(const char* path)
{
    DirectoryRecord* entry = GetEntry(path);
    if(entry == 0 || GetEntryType(entry) == Iso_File)
    {
        return false;
    }

    return true;
}