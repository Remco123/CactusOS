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
    Log(Info, "Initializing FAT Filesystem");

    FAT32_BPB bpb;
    if(this->disk->ReadSector(this->StartLBA, (uint8_t*)&bpb) != 0)
        return false;

    this->bytesPerSector = bpb.bytesPerSector;
    this->rootDirSectors = ((bpb.NumDirEntries * 32) + (this->bytesPerSector - 1)) / this->bytesPerSector;
    this->sectorsPerCluster = bpb.SectorsPerCluster;
    this->firstFatSector = bpb.ReservedSectors;
    this->clusterSize = this->bytesPerSector * this->sectorsPerCluster;

    // Allocate Read Buffer
    this->readBuffer = new uint8_t[this->bytesPerSector];

    // Size of one FAT in clusters
    uint32_t FatSize = bpb.SectorsPerFat12_16 != 0 ? bpb.SectorsPerFat12_16 : bpb.SectorsPerFat32;
    
    // Calculate first data sector
    this->firstDataSector = bpb.ReservedSectors + (bpb.NumOfFats * FatSize) + this->rootDirSectors;
    
    // Total count of sectors used for the entire filesystem
    uint32_t TotalSectors = bpb.TotalSectorsSmall != 0 ? bpb.TotalSectorsSmall : bpb.TotalSectorsBig;

    // How much sectors does the complete data region have in use?
    uint32_t DataSectors = TotalSectors - (bpb.ReservedSectors + (bpb.NumOfFats * FatSize) + this->rootDirSectors);

    // Total amount of clusters, clusters are only used in the data area
    this->totalClusters = DataSectors / this->sectorsPerCluster;

    // Now we can determine the type of filesystem we are dealing with
    if(this->totalClusters < 4085) {
        this->FatType = FAT12;
        this->FatTypeString = "FAT12";
    } else if(this->totalClusters < 65525) {
        this->FatType = FAT16;
        this->FatTypeString = "FAT16";
    } else {
        this->FatType = FAT32;
        this->FatTypeString = "FAT32";
    }

    if(this->FatType == FAT12 || this->FatType == FAT16)
        this->rootDirCluster = bpb.ReservedSectors + (bpb.NumOfFats * FatSize);
    else
        this->rootDirCluster = bpb.RootDirCluster;

#if 1
    Log(Info, "%s Filesystem Summary: ", this->FatTypeString);
    Log(Info, "      Bytes Per Sector: %d", this->bytesPerSector);
    Log(Info, "          Root Sectors: %d", this->rootDirSectors);
    Log(Info, "       Sectors/Cluster: %d", this->sectorsPerCluster);
    Log(Info, "     First Data Sector: %d", this->firstDataSector);
    Log(Info, "      Reserved Sectors: %d", bpb.ReservedSectors);
#endif

#if 1
    auto rootEntries = this->GetDirectoryEntries(this->rootDirCluster, true);
    for(FATEntryInfo e : rootEntries) {
        Log(Info, (char*)e.filename);
    }
#endif

    return true;
}

inline uint32_t FAT::ClusterToSector(uint32_t cluster) {
    return ((cluster - 2) * this->sectorsPerCluster) + this->firstDataSector;
}


uint32_t FAT::ReadTable(uint32_t cluster)
{
    if(cluster < 2 || cluster > this->totalClusters) {
        Log(Error, "%s invallid cluster number %d", this->FatTypeString, cluster);
        return 0;
    }
    
    if(this->FatType == FAT32)
    {
        uint32_t fatOffset = cluster * 4;
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return 0;
        
        //remember to ignore the high 4 bits.
        return *(uint32_t*)&this->readBuffer[entOffset] & 0x0FFFFFFF;
    }
    else if(this->FatType == FAT16)
    {
        uint32_t fatOffset = cluster * 2;
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return 0;
        
        return *(uint16_t*)&this->readBuffer[entOffset];
    }
    else // FAT12
    {
        uint32_t fatOffset = cluster + (cluster / 2); // multiply by 1.5
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return 0;
        
        uint16_t tableValue = *(uint16_t*)&this->readBuffer[entOffset];
        
        if(cluster & 0x0001)
            tableValue = tableValue >> 4;
        else
            tableValue = tableValue & 0x0FFF;

        return tableValue;
    }
}

void FAT::WriteTable(uint32_t cluster, uint32_t value)
{
    if(cluster < 2 || cluster > this->totalClusters) {
        Log(Error, "%s invallid cluster number %d", this->FatTypeString, cluster);
        return;
    }

    if(this->FatType == FAT32)
    {
        uint32_t fatOffset = cluster * 4;
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return;
        
        *(uint32_t*)&this->readBuffer[entOffset] = value;

        if(this->disk->WriteSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            Log(Error, "Could not write new FAT value for cluster %d", cluster);
    }
    else if(this->FatType == FAT16)
    {
        uint32_t fatOffset = cluster * 2;
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return;
        
        *(uint16_t*)&this->readBuffer[entOffset] = (uint16_t)value;

        if(this->disk->WriteSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            Log(Error, "Could not write new FAT value for cluster %d", cluster);       
    }
    else // FAT12
    {
        uint32_t fatOffset = cluster + (cluster / 2); // multiply by 1.5
        uint32_t fatSector = this->firstFatSector + (fatOffset / this->bytesPerSector);
        uint32_t entOffset = fatOffset % this->bytesPerSector;
        
        if(this->disk->ReadSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            return;
        
        if(cluster & 0x0001) {
            value = value << 4;	/* Cluster number is ODD */
            *((uint16_t*)(&this->readBuffer[entOffset])) = (*((uint16_t*)(&this->readBuffer[entOffset]))) & 0x000F;
        } else {
            value = value & 0x0FFF;	/* Cluster number is EVEN */
            *((uint16_t*)(&this->readBuffer[entOffset])) = (*((uint16_t*)(&this->readBuffer[entOffset]))) & 0xF000;
        }
        *((uint16_t*)(&this->readBuffer[entOffset])) = (*((uint16_t*)(&this->readBuffer[entOffset]))) | value;


        if(this->disk->WriteSector(this->StartLBA + fatSector, this->readBuffer) != 0)
            Log(Error, "Could not write new FAT value for cluster %d", cluster);  
    }
}

// This could be optimized a lot, perhaps using the FSInfo Structure
uint32_t FAT::AllocateCluster()
{
	uint32_t freeCluster = CLUSTER_FREE;
	uint32_t endCluster = CLUSTER_END;

	uint32_t cluster = 2; // Start at offset 2

	//Iterate through the clusters, looking for a free cluster
	while (cluster < this->totalClusters)
	{
	    uint32_t value = ReadTable(cluster);

		if (value == freeCluster) { //cluster found, allocate it.
			WriteTable(cluster, endCluster);
			return cluster;
        }

		cluster++; //cluster is taken, check the next one
	}

	return 0;
}

// Parse a directory and return its entries
List<FATEntryInfo> FAT::GetDirectoryEntries(uint32_t dirCluster, bool rootDirectory)
{
    List<FATEntryInfo> results;

    uint32_t sector = 0;    
    uint32_t cluster = dirCluster;

    while ((cluster != CLUSTER_FREE) && (cluster < CLUSTER_END))
    {
        /*
        With FAT12 and FAT16 the root directory is positioned after the File Allocation Table
        This is calculated below
        FAT32 does not use this technique
        */

        if(rootDirectory && this->FatType != FAT32)
            sector = this->firstDataSector - this->rootDirSectors;
        else
            sector = ClusterToSector(dirCluster);

        List<LFNEntry> lfnEntries;
        for(uint16_t i = 0; i < this->sectorsPerCluster; i++) // Loop through sectors in this cluster
        {
            if(this->disk->ReadSector(this->StartLBA + sector + i, this->readBuffer) != 0) {
                Log(Error, "Error reading disk at lba %d", this->StartLBA + sector + i);
                return results;
            }

            for(uint8_t entryIndex = 0; entryIndex < (this->bytesPerSector / sizeof(DirectoryEntry)); entryIndex++) // Loop through entries in this sector
            {
                DirectoryEntry* entry = (DirectoryEntry*)(this->readBuffer + entryIndex * sizeof(DirectoryEntry));

                if(entry->FileName[0] == ENTRY_END) // End of entries
                    return results;
                
                if(entry->FileName[0] == ENTRY_UNUSED) // Unused entry, probably deleted or something
                    continue; // Just skip this entry

                if(entry->FileName[0] == 0x2E) // . or .. entry
                    continue;

                if(entry->FileName[0] == 0x05) // Pending file to delete apparently
                    continue;

                if(entry->Attributes == ATTR_VOLUME_ID) // Volume ID of filesystem
                    continue;
                
                if(entry->Attributes == ATTR_LONG_NAME) { // Long file name entry
                    LFNEntry* lfn = (LFNEntry*)entry; // Turn the directory entry into a LFNEntry using the magic of pointers
                    lfnEntries.push_back(*lfn); // Add it to our buffer
                    continue;
                }

                // This is a valid entry, so add it to our list
                FATEntryInfo item;
                item.entry = *entry;
                if(lfnEntries.size() > 0) { // We have some LFN entries in our list that belong to this entry
                    item.filename = ParseLFNEntries(lfnEntries, *entry);
                    lfnEntries.Clear();
                }
                else
                    item.filename = ParseShortFilename((char*)entry->FileName);
                
                results.push_back(item);
            }
        }

        if(rootDirectory && this->FatType != FAT32) {
            Log(Error, "FAT Root directory has more than 224 entries, this should not be possible!");
            return results;
        }

        cluster = ReadTable(cluster);
        Log(Info, "Next cluster is %x", cluster);
    }
    Log(Error, "This should not be reached %s %d", __FILE__, __LINE__);
    return results;
}

#pragma region Filename Conversion

// Calculate checksum for 8.3 filename
uint8_t FAT::Checksum(char* filename)
{
	uint8_t Sum = 0;
	for (uint8_t len = 11; len != 0; len--) {
		// NOTE: The operation is an unsigned char rotate right
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *filename++;
	}
	return Sum;
}

// Parse a list of long file name entries, also pass the 8.3 entry for the checksum
char* FAT::ParseLFNEntries(List<LFNEntry> entries, DirectoryEntry sfnEntry)
{
    // Calculate checksum of short file name
    uint8_t shortChecksum = Checksum((char*)sfnEntry.FileName);

    // Allocate space for complete name
    char* longName = new char[entries.size() * 13 + 1]; // Each LFN holds 13 characters + one for termination
    MemoryOperations::memset(longName, 0, entries.size() * 13 + 1);

    for(LFNEntry item : entries)
    {
        if(item.checksum != shortChecksum) {
            Log(Error, "Checksum of LFN entry is incorrect");
            return 0;
        }

        uint8_t index = item.entryIndex & 0x0F;
        char* namePtr = longName + ((index - 1) * 13);

        // First part of filename
        for(int i = 0; i < 9; i+=2) {
            if(item.namePart1[i] >= 32 && item.namePart1[i] <= 127) // Valid character
                *namePtr = item.namePart1[i];
            else
                *namePtr = 0;
            namePtr++;
        }

        // Second part of filename
        for(int i = 0; i < 11; i+=2) {
            if(item.namePart2[i] >= 32 && item.namePart2[i] <= 127) // Valid character
                *namePtr = item.namePart2[i];
            else
                *namePtr = 0;
            namePtr++;
        }

        // Third part of filename
        for(int i = 0; i < 3; i+=2) {
            if(item.namePart3[i] >= 32 && item.namePart3[i] <= 127) // Valid character
                *namePtr = item.namePart3[i];
            else
                *namePtr = 0;
            namePtr++;
        }
    }
    return longName;
}

// Turn a FAT filename into a readable one
char* FAT::ParseShortFilename(char* str)
{
    char* outFileName = new char[12];
    MemoryOperations::memset(outFileName, 0, 12);

    int mainEnd, extEnd;
	for(mainEnd = 8; mainEnd > 0 && str[mainEnd - 1] == ' '; mainEnd--);

	MemoryOperations::memcpy(outFileName, str, mainEnd);

	for(extEnd = 3; extEnd > 0 && str[extEnd - 1 + 8] == ' '; extEnd--);

	if(extEnd == 0)
		return String::Lowercase(outFileName);

	outFileName[mainEnd] = '.';
	MemoryOperations::memcpy(outFileName + mainEnd + 1, (const char*)str + 8, extEnd);
    
    return String::Lowercase(outFileName);
}

#pragma endregion











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