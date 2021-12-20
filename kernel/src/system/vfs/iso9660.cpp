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
    BootConsole::WriteLine("ISO9660 Filesystem Intialized");
    BootConsole::ForegroundColor = oldColor;
    return true;
}

char* ISO9660::GetRecordName(DirectoryRecord* record)
{
    if(record->name[0] == '\0' || record->name[0] == '\1') // . and .. entries
        return 0;

    // Search for Rockridge extension first
    if(record->name[record->name_length + 1] != 0) {
        int offset = record->name_length + 1;
        if(offset % 2 == 0) // Even number
            offset -= 1;
        
        while (1)
        {
            int len = record->name[offset + 2];
            if(String::strncmp(record->name + offset, "NM", 2)) // Name field
            {
                int strLen = len - 5;
                char* entry = new char[strLen + 1];
                MemoryOperations::memcpy(entry, record->name + offset + 5, strLen);
                entry[strLen] = '\0';
                
                return entry;
            }
            else
                if(record->name[offset] == 0)
                    break;
                else
                    offset += len;
        }
    }
    
    // Return short name, could be enough
    if(GetEntryType(record) == Iso_File)
    {
        char* entry = new char[record->name_length];
        MemoryOperations::memcpy(entry, record->name, record->name_length - 2);
        entry[record->name_length - 2] = '\0';
        
        return entry;
    }
    else //Directories do not have version numbers
    {
        char* entry = new char[record->name_length + 1];
        MemoryOperations::memcpy(entry, record->name, record->name_length);
        entry[record->name_length] = '\0';
        
        return entry;
    }
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
        // TODO: there has to be a better way
        if(CDROM_SECTOR_SIZE - Offset < (sizeof(DirectoryRecord) - sizeof(DirectoryRecord::name))) //We reached the end of the sector, so we need to read the next one
        {
            Offset = 0; // Reset the offset in the sector
            if(this->disk->ReadSector(searchIn->extent_location + SectorOffset, readBuffer) != 0)
                return 0;
            SectorOffset++;
        }

        DirectoryRecord* record = (DirectoryRecord*)(readBuffer + Offset);
        if(record->length == 0) // We reached the end of the entry's
        {
            break;
        }
        else
        {
            char* entryName = GetRecordName(record);

            if(entryName != 0 && String::strcmp(entryName, name))
            {
                DirectoryRecord* result = new DirectoryRecord();
                MemoryOperations::memcpy(result, record, sizeof(DirectoryRecord));

                //Terminate string
                result->name[record->name_length] = '\0';

                //Free memory
                delete entryName;
                
                return result;
            }
            if(entryName != 0)
                delete entryName;
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

List<LIBCactusOS::VFSEntry>* ISO9660::DirectoryList(const char* path)
{
    List<LIBCactusOS::VFSEntry>* result = new List<LIBCactusOS::VFSEntry>();
    DirectoryRecord* parent = String::strlen(path) > 0 ? GetEntry(path) : rootDirectory;
    if(parent == 0 || GetEntryType(parent) == Iso_File)
        return result;

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
            break;
        else
            if(record->name[0] != '\0' && record->name[0] != '\1') //We ignore the . and .. directories
            {
                char* entryName = GetRecordName(record);
                if(entryName != 0) {
                    LIBCactusOS::VFSEntry entry;
                    MemoryOperations::memset(&entry, 0, sizeof(LIBCactusOS::VFSEntry));

                    // Fill in the info
                    entry.size = record->data_length;
                    entry.isDir = (GetEntryType(record) == Iso_EntryType::Iso_Directory);
                    entry.creationDate.day = record->datetime[2];
                    entry.creationDate.month = record->datetime[1];
                    entry.creationDate.year = record->datetime[0] + 1900;

                    entry.creationTime.sec = record->datetime[5];
                    entry.creationTime.min = record->datetime[5];
                    entry.creationTime.hour = record->datetime[3];

                    int len = String::strlen(entryName);
                    MemoryOperations::memcpy(entry.name, entryName, len < VFS_NAME_LENGTH ? len : VFS_NAME_LENGTH);

                    result->push_back(entry);
                }
            }

        Offset += record->length;
    }

    if(parent != 0 && parent != this->rootDirectory)
        delete parent;
        
    return result;
}
uint32_t ISO9660::GetFileSize(const char* path)
{
    DirectoryRecord* entry = GetEntry(path);
    if(entry == 0 || GetEntryType(entry) == Iso_Directory)
    {
        if(entry != 0) delete entry;
        return -1;
    }
    uint32_t len = entry->data_length;

    delete entry;
    return len;
}
int ISO9660::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{
    DirectoryRecord* entry = GetEntry(path);

    if(entry == 0 || GetEntryType(entry) == Iso_Directory) {
        if(entry != 0) delete entry;
        return -1;
    }

    if(len == (uint32_t)-1)
        len = entry->data_length;

    //TODO: Actually implement partial file reading, for now we read the whole file
    int sectorCount = entry->data_length / CDROM_SECTOR_SIZE;
    int dataRemainder = entry->data_length % CDROM_SECTOR_SIZE;

    for(int i = 0; i < sectorCount; i++)
        if(this->disk->ReadSector(entry->extent_location + i, buffer + (CDROM_SECTOR_SIZE * i)) != 0) {
            delete entry;
            return -1;
        }
    
    if(dataRemainder > 0) //We have a remainder
    {
        if(this->disk->ReadSector(entry->extent_location + sectorCount, readBuffer) != 0) {
            delete entry;
            return -1;
        }
        MemoryOperations::memcpy(buffer + (sectorCount*CDROM_SECTOR_SIZE), readBuffer, dataRemainder);
    }
    
    delete entry;
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
        if(entry != 0) delete entry;
        return false;
    }

    delete entry;
    return true;
}

bool ISO9660::DirectoryExists(const char* path)
{
    DirectoryRecord* entry = GetEntry(path);
    if(entry == 0 || GetEntryType(entry) == Iso_File)
    {
        if(entry != 0) delete entry;
        return false;
    }

    delete entry;
    return true;
}