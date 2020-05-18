#include <system/vfs/fat32.h>

#include <common/print.h>
#include <system/log.h>
#include <system/system.h>

#define DECLARE_LOCK(name) volatile int name ## Locked
#define LOCK(name) \
    while (name ## Locked == 1) asm("pause"); \
    __sync_synchronize();
#define UNLOCK(name) \
    __sync_synchronize(); \
    name ## Locked = 0;

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

DECLARE_LOCK(readBuffer);

FAT32::FAT32(Disk* disk, uint32_t start, uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->Name = "FAT32 Filesystem";
}

bool FAT32::Initialize()
{
    BootConsole::WriteLine();
    BootConsole::WriteLine("Initializing FAT Filesystem");
    MemoryOperations::memset(readBuffer, 0, 512);

    if(disk->ReadSector(this->StartLBA, readBuffer) != 0)
        return false;

    FAT32_BPB* biosParameterBlock = (FAT32_BPB*)readBuffer;

    //Check if filesystem is valid
    if(biosParameterBlock->bytesPerSector != 512 || biosParameterBlock->BootSignature != 0xAA55)
        return false;

    //Calculate variables from bpb
    uint32_t totalSectors = (biosParameterBlock->TotalSectorsSmall == 0) ? biosParameterBlock->TotalSectorsBig : biosParameterBlock->TotalSectorsSmall;
    uint32_t fatSize = (biosParameterBlock->SectorsPerFat12_16 == 0) ? biosParameterBlock->SectorsPerFat32 : biosParameterBlock->SectorsPerFat12_16;
    uint32_t rootDirSectors = ((biosParameterBlock->NumDirEntries * 32) + (biosParameterBlock->bytesPerSector - 1)) / biosParameterBlock->bytesPerSector;
    uint32_t firstDataSector = biosParameterBlock->ReservedSectors + (biosParameterBlock->NumOfFats * fatSize) + rootDirSectors;
    uint32_t firstFatSector = biosParameterBlock->ReservedSectors;
    uint32_t dataSectors = totalSectors - firstDataSector;
    uint32_t totalClusters = dataSectors / biosParameterBlock->SectorsPerCluster;

    //And save some vars
    this->bytesPerSector = biosParameterBlock->bytesPerSector;
    this->totalClusters = totalClusters;
    this->sectorsPerCluster = biosParameterBlock->SectorsPerCluster;
    this->firstFatSector = firstFatSector;
    this->firstDataSector = firstDataSector;
    this->reservedSectors = biosParameterBlock->ReservedSectors;
    this->rootCluster = biosParameterBlock->RootDirCluster;

    //Check if this is a fat32 volume, otherwise return
    if(totalClusters < 4085) 
        return false;
    else if(totalClusters < 65525) 
        return false;
    else if (totalClusters < 268435445)
        BootConsole::WriteLine("Filesystem is fat32");
    else
        return false;

    //Print some variables
    BootConsole::Write("Bytes per Sector: "); BootConsole::WriteLine(Convert::IntToString(bytesPerSector));
    BootConsole::Write("Sectors per Cluster: "); BootConsole::WriteLine(Convert::IntToString(sectorsPerCluster));
    BootConsole::Write("Root dir cluster: "); BootConsole::WriteLine(Convert::IntToString(biosParameterBlock->RootDirCluster));

    /*
    BootConsole::WriteLine("### Root Directory ###");
    auto childs = DirectoryList("");
    for(int i = 0; i < childs->size(); i++) {
        BootConsole::WriteLine(childs->GetAt(i));
        delete childs->GetAt(i);
    }
    */

    uint8_t oldColor = BootConsole::ForegroundColor;
    BootConsole::ForegroundColor = VGA_COLOR_GREEN;
    BootConsole::Write("FAT32 Filesystem Intialized");
    BootConsole::ForegroundColor = oldColor;
    return true;
}

FAT_DATE FAT32::CurrentDate()
{
    FAT_DATE ret;
    ret.day = System::rtc->GetDay();
    ret.mon = System::rtc->GetMonth();
    ret.year = System::rtc->GetYear() - 1980;
    return ret;
}

FAT_TIME FAT32::CurrentTime()
{
    FAT_TIME ret;
    ret.hour = System::rtc->GetHour();
    ret.min = System::rtc->GetMinute();
    ret.sec = System::rtc->GetSecond() / 2;
}

////////////////
// Table Functions
////////////////

uint32_t FAT32::ReadFATTable(uint32_t cluster)
{
    if(cluster < 2 || cluster >= totalClusters) {
        Log(Error, "Cluster number is out of range 2 < %d < %d", cluster, totalClusters);
        return 0;
    }

    uint32_t cluster_size = bytesPerSector * sectorsPerCluster;
	uint32_t fatOffset = cluster * 4;
	uint32_t fatSector = firstFatSector + (fatOffset / cluster_size);
	uint32_t entOffset = fatOffset % cluster_size;

    LOCK(readBuffer);
    if(this->disk->ReadSector(this->StartLBA + fatSector, readBuffer) != 0) {
        UNLOCK(readBuffer);
        return 0;
    }
    
    uint32_t value = *(uint32_t*)&readBuffer[entOffset] & 0x0FFFFFFF;

    UNLOCK(readBuffer);
    return value;
}

int FAT32::WriteFATTable(uint32_t cluster, uint32_t value)
{
    if(cluster < 2 || cluster >= totalClusters) {
        Log(Error, "Cluster number is out of range 2 < %d < %d", cluster, totalClusters);
        return -1;
    }

    uint32_t cluster_size = bytesPerSector * sectorsPerCluster;
	uint32_t fatOffset = cluster * 4;
	uint32_t fatSector = firstFatSector + (fatOffset / cluster_size);
	uint32_t entOffset = fatOffset % cluster_size;

    LOCK(readBuffer);
    if(this->disk->ReadSector(this->StartLBA + fatSector, readBuffer) != 0) {
        UNLOCK(readBuffer);
        return -1;
    }

    *(uint32_t*)&readBuffer[entOffset] = value;

    if(this->disk->WriteSector(this->StartLBA + fatSector, readBuffer) != 0) {
        UNLOCK(readBuffer);
        return -1;
    }

    UNLOCK(readBuffer);
    return 0;
}

uint32_t FAT32::AllocateNewCluster()
{
    uint32_t cluster = 2;
    while (cluster < totalClusters) {
        uint32_t value = ReadFATTable(cluster);
        if(value == FAT_CLUSTER_FREE) {
            // Allocate cluster
            WriteFATTable(cluster, FAT_CLUSTER_END);

            return cluster;
        }
        else if((int)value < 0) {
            Log(Error, "Error getting status of cluster %d", cluster);
            return 0;
        }
        cluster++;
    }
}

List<DirectoryEntry> FAT32::ReadDir(uint32_t cluster)
{ 
	// Get the cluster chain of the directory. This contains the directory entries themselves.
	uint64_t numclus = 0;	
	List<uint32_t> clusters = this->GetClusterChain(cluster, &numclus);
    List<DirectoryEntry> result;
 
	// allocate a buffer, rounding up the size to a page.
	uint8_t* buf = new uint8_t[numclus * this->sectorsPerCluster * 512];
	uint8_t* obuf = buf;
    
    //Loop through the clusters and load them into buf
	for (uint32_t cluster : clusters) {
		for(uint32_t s = 0; s < this->sectorsPerCluster; s++)
        {
            uint32_t sectorToRead = CLUSTER_TO_SECTOR(cluster) + s;
            if(this->disk->ReadSector(this->StartLBA + sectorToRead, buf) != 0) {
                delete buf;
                return result;
            }

            buf += this->bytesPerSector;
        }
    }
	buf = obuf;
 
	uint64_t numDirEntries = (numclus * this->sectorsPerCluster * 512) / sizeof(DirectoryEntry);
    for(uint64_t i = 1; i < numDirEntries; i++)
    {
        DirectoryEntry* dirEntry = (DirectoryEntry*)(buf + i*sizeof(DirectoryEntry));

        if(dirEntry->FileName[0] == FAT_ENDOFDIRS)
            break;
        
        if(dirEntry->FileName[0] == FAT_UNUSED)
            continue;

        if(dirEntry->Attributes == FAT_SYSTEM || dirEntry->Attributes == FAT_HIDDEN || dirEntry->Attributes == FAT_LFN)
            continue;
        
        //It is a valid entry
        //BootConsole::Write("Found entry: "); BootConsole::WriteLine((char*)dirEntry->FileName);
        result.push_back(*dirEntry);
    }
    return result;
}

List<uint32_t> FAT32::GetClusterChain(uint32_t firstcluster, uint64_t* numclus)
{
	// setup some stuff.
	uint32_t Cluster = firstcluster;
	uint32_t cchain = 0;
	List<uint32_t> ret;
 
	// here we assume your 'ReadFromDisk' only does 512 bytes at a time or something.
	do
	{ 
		// using FatOffset, we just index into the array to get the value we want.
		cchain = ReadFATTable(Cluster);
 
		// the value of 'Cluster' will change by the next iteration.
		// Because we're nice people, we need to include the first cluster in the list of clusters we return.
		ret.push_back(Cluster);
 
		// since cchain is the next cluster in the list, we just modify the things, shouldn't be too hard to grasp.
		Cluster = cchain;
 
		// numclus tells the caller how many clusters are in the chain.
		(*numclus)++;
 
	} while((cchain != 0) && !((cchain & 0x0FFFFFFF) >= 0x0FFFFFF8));	
 
	// check that there's more entries in the chain, if not break the loop.
	// the reason for using a do-while should be obvious.
 
	return ret;
}

DirectoryEntry FAT32::SearchInDirectory(uint32_t cluster, const char* name)
{
    List<DirectoryEntry> childs = ReadDir(cluster);

    DirectoryEntry result;
    MemoryOperations::memset(&result, 0, sizeof(DirectoryEntry));

    for(int i = 0; i < childs.size(); i++)
        if(String::strncmp((char*)childs[i].FileName, name, FAT_FILENAME_LEN)) {
            result = childs[i];
            break;
        }

    return result;
}

DirectoryEntry FAT32::GetEntry(const char* path)
{
    uint32_t searchCluster = this->rootCluster;
    DirectoryEntry err;
    err.FileName[0] = '\0';

    List<char*> pathList = String::Split(path, PATH_SEPERATOR_C);

    if(pathList.size() == 0) {//The path represents a entry in the root directory, for example just: "test.txt"
        char* name = ToFatFormat((char*)path);
        DirectoryEntry entry = SearchInDirectory(this->rootCluster, name);
        delete name;
        
        return entry;
    }
    for(int i = 0; i < pathList.size(); i++)
    {
        char* name = ToFatFormat(pathList[i]);
        DirectoryEntry entry = this->SearchInDirectory(searchCluster, name);
        delete name;

        if(entry.FileName[0] == 0) {
            return err;
        }
        else if(i == pathList.size() - 1)
        {
            return entry;
        }

        searchCluster = GET_CLUSTER((&entry));
    }

    return err;
}

int FAT32::CreateEntry(const char* path, char type)
{
    uint32_t parentDirectoryCluster = 0;
    List<char*> pathList = String::Split(path, PATH_SEPERATOR_C);
    char* dirName = (pathList.size() > 0) ? pathList[pathList.size() - 1] : (char*)path;
    dirName = ToFatFormat(dirName);

    if(pathList.size() == 0)
        parentDirectoryCluster = this->rootCluster;
    else {
        char* directoryString = new char[String::strlen(path)];
        int i = String::IndexOf(path, PATH_SEPERATOR_C, pathList.size() - 2);
        MemoryOperations::memcpy(directoryString, path, i);

        DirectoryEntry entry = GetEntry(directoryString);
        if(entry.FileName[0] == '\0' || entry.Attributes != FAT_DIRECTORY)
            return -1;
        
        parentDirectoryCluster = GET_CLUSTER((&entry));
    }

    LOCK(readBuffer);
    if(this->disk->ReadSector(this->StartLBA + CLUSTER_TO_SECTOR(parentDirectoryCluster), readBuffer) != 0) {
        UNLOCK(readBuffer);
        return -1;
    }
    
    DirectoryEntry* entryPtr = (DirectoryEntry*)readBuffer;
    while(1) {
        if(entryPtr->FileName[0] != FAT_ENDOFDIRS && entryPtr->FileName[0] != FAT_UNUSED) // This slot is not free
            if(((uint32_t)entryPtr + sizeof(DirectoryEntry)) > ((uint32_t)readBuffer + 512)) { // We should read the next cluster here. TODO: Implement
                BootConsole::WriteLine("FAT Error");
                UNLOCK(readBuffer);
                return -1;
            }
            else
                entryPtr++;
        else {
            // Create new entry
            DirectoryEntry newDirectory;
            MemoryOperations::memset(&newDirectory, 0, sizeof(DirectoryEntry));

            newDirectory.Attributes = type;
            newDirectory.FileSize = 0;
            newDirectory.CDate = CurrentDate().intValue;
            newDirectory.CTime = CurrentTime().intValue;
            newDirectory.CTimeTenthSecs = 0;
            newDirectory.ADate = newDirectory.CDate;
            newDirectory.MDate = newDirectory.CDate;
            newDirectory.MTime = newDirectory.CTime;

            MemoryOperations::memcpy(newDirectory.FileName, dirName, FAT_FILENAME_LEN);

            uint32_t newCluster = AllocateNewCluster();
            if(newCluster == 0) {
                Log(Error, "Could not allocate a new cluster for directory");
                UNLOCK(readBuffer);
                return 0;
            }

            newDirectory.LowFirstCluster = newCluster & 0xFF;
            newDirectory.HighFirstCluster = newCluster >> 16;

            // Copy directory back to readbuffer
            MemoryOperations::memcpy((void*)((uint32_t)entryPtr), &newDirectory, sizeof(DirectoryEntry));

            // And write it back to the disk
            if(this->disk->WriteSector(this->StartLBA + CLUSTER_TO_SECTOR(parentDirectoryCluster), readBuffer) != 0) {
                UNLOCK(readBuffer);
                return -1;
            }             
            
            UNLOCK(readBuffer);
            return 0;
        }
    }
}



List<char*>* FAT32::DirectoryList(const char* path)
{ 
    List<char*>* result = new List<char*>();

    uint32_t targetCluster = this->rootCluster;
    if(String::strlen(path) > 0)
    {
        DirectoryEntry entry = GetEntry(path);

        targetCluster = GET_CLUSTER((&entry));
    }

    List<DirectoryEntry> rawChilds = ReadDir(targetCluster);
    for(DirectoryEntry entry : rawChilds)
        result->push_back(ToNormalFormat((char*)entry.FileName));

    return result;
}
uint32_t FAT32::GetFileSize(const char* path)
{
    DirectoryEntry entry = GetEntry(path);
    if(entry.FileName[0] == '\0' || entry.Attributes == FAT_DIRECTORY)
    {
        return -1;
    }

    return entry.FileSize;
}
int FAT32::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{ 
    uint8_t* bufferPtr = buffer;

    DirectoryEntry entry = GetEntry(path);
    if(entry.FileName[0] == '\0' || entry.Attributes == FAT_DIRECTORY)
    {
        return -1;
    }

    uint32_t bytesRead = 0;

    uint32_t beginCluster = GET_CLUSTER((&entry));

    uint64_t numclus = 0;	
	List<uint32_t> clusters = this->GetClusterChain(beginCluster, &numclus);

    uint8_t* obuf = buffer;
    
    //Loop through the clusters and load them into buf
	for (int i = 0; i < clusters.size(); i++) {
        uint32_t cluster = clusters[i];
		
		for(uint32_t s = 0; s < this->sectorsPerCluster && (i*sectorsPerCluster*bytesPerSector + s*bytesPerSector) < entry.FileSize; s++) {
            uint32_t sectorToRead = CLUSTER_TO_SECTOR(cluster) + s;
            LOCK(readBuffer);
            if(this->disk->ReadSector(this->StartLBA + sectorToRead, readBuffer) != 0) {
                UNLOCK(readBuffer);
                return -1;
            }

            uint32_t remaingBytes = entry.FileSize - bytesRead;

            //Copy the required part of the buffer
            MemoryOperations::memcpy(buffer, readBuffer, remaingBytes <= 512 ? remaingBytes : 512);
            UNLOCK(readBuffer);

            bytesRead += 512;
            buffer += this->bytesPerSector;
        }
	}
	buffer = obuf;

    return 0;
}

int FAT32::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create)
{
    
}

int FAT32::CreateFile(const char* path)
{
    return CreateEntry(path, 0);
}

int FAT32::CreateDirectory(const char* path)
{
    return CreateEntry(path, FAT_DIRECTORY);
}

bool FAT32::FileExists(const char* path)
{ 
    DirectoryEntry entry = GetEntry(path);
    if(entry.FileName[0] == '\0' || entry.Attributes == FAT_DIRECTORY)
    {
        return false;
    }

    return true;
}
bool FAT32::DirectoryExists(const char* path)
{ 
    DirectoryEntry entry = GetEntry(path);
    if(entry.FileName[0] == '\0' || entry.Attributes != FAT_DIRECTORY)
    {
        return false;
    }

    return true;
}

////////////
// Filename Conversion
////////////


inline uint8_t toupper(uint8_t c)
{
    return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}

char* FAT32::ToFatFormat(char* str)
{
    char* outFileName = new char[12];

    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; str[i] && str[i] != '.'; i++ )
        outFileName[i] = toupper(str[i]);

    j = i;

    for (; i < 8; i++ )
        outFileName[i] = ' ';

    if ( str[j] == '.' )
        for ( j++; str[j]; i++, j++ )
            outFileName[i] = toupper(str[j]);

    for (; i < FAT_FILENAME_LEN; i++ )
        outFileName[i] = ' ';

    outFileName[i] = 0;

    return outFileName;
}

char* FAT32::ToNormalFormat(char* str)
{
    char* outFileName = new char[12];
    MemoryOperations::memset(outFileName, 0, 12);

    int mainEnd, extEnd;
	for(mainEnd = 8; mainEnd > 0 && str[mainEnd - 1] == ' '; mainEnd--);

	MemoryOperations::memcpy(outFileName, str, mainEnd);

	for(extEnd = 3; extEnd > 0 && str[extEnd - 1 + 8] == ' '; extEnd--);

	if(extEnd == 0){
		return String::Lowercase(outFileName);
	}

	outFileName[mainEnd] = '.';
	MemoryOperations::memcpy(outFileName + mainEnd + 1, (const char*)str + 8, extEnd);
    
    return String::Lowercase(outFileName);
}