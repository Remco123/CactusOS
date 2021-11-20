#include <system/vfs/fat.h>

#include <system/log.h>
#include <system/system.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

FAT::FAT(Disk* disk, uint32_t start, uint32_t size)
: VirtualFileSystem(disk, start, size) 
{
    this->Name = "FAT Filesystem";
    MemoryOperations::memset(&this->fsInfo, 0, sizeof(FAT32_FSInfo));
}

FAT::~FAT()
{
    delete this->readBuffer;
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

    // Check for FSInfo structure and read it into this->fsInfo
    if(this->FatType == FAT32 && bpb.FSInfoSector > 0) {
        if(this->disk->ReadSector(this->StartLBA + bpb.FSInfoSector, (uint8_t*)&this->fsInfo) != 0)
            return false;
    }

#if 1
    Log(Info, "%s Filesystem Summary: ", this->FatTypeString);
    Log(Info, "      Bytes Per Sector: %d", this->bytesPerSector);
    Log(Info, "          Root Sectors: %d", this->rootDirSectors);
    Log(Info, "       Sectors/Cluster: %d", this->sectorsPerCluster);
    Log(Info, "     First Data Sector: %d", this->firstDataSector);
    Log(Info, "      Reserved Sectors: %d", bpb.ReservedSectors);
    if(this->FatType == FAT32) {
        Log(Info, "  FSInfo Free Clusters: %x", this->fsInfo.lastFreeCluster);
        Log(Info, "   FSInfo Start Search: %x", this->fsInfo.startSearchCluster);
    }
#endif

    if(this->fsInfo.startSearchCluster == 0xFFFFFFFF)   // Unkown
            this->fsInfo.startSearchCluster = 2;        // Then we use the default value

    else if(this->FatType == FAT12 || this->FatType == FAT16)
        this->fsInfo.startSearchCluster = 2; // Might as well still use this variable for FAT12/FAT16

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

uint32_t FAT::AllocateCluster()
{
    // Use start cluster from fsInfo, this is also valid for FAT12/FAT16 thanks to some magic.
	uint32_t cluster = this->fsInfo.startSearchCluster;

	//Iterate through the clusters, looking for a free cluster
	while (cluster < this->totalClusters)
	{
	    uint32_t value = ReadTable(cluster);

		if (value == CLUSTER_FREE) {                    // Cluster found, allocate it.
            this->fsInfo.startSearchCluster = cluster;  // Update fsInfo structure
			WriteTable(cluster, CLUSTER_END);           // Write EOC to the cluster
			return cluster;
        }

		cluster++; //cluster is taken, check the next one
	}
	return 0;
}

void FAT::ClearCluster(uint32_t cluster)
{
    uint32_t sector = ClusterToSector(cluster);
    MemoryOperations::memset(this->readBuffer, 0, this->bytesPerSector);

    // Clear each sector of cluster
    for(uint8_t i = 0; i < this->sectorsPerCluster; i++)
        if(this->disk->WriteSector(this->StartLBA + sector + i, this->readBuffer) != 0) {
            Log(Error, "Could not clear sector %d of cluster %d", sector, cluster);
            return;
        }
}

// Parse a directory and return its entries
List<FATEntryInfo> FAT::GetDirectoryEntries(uint32_t dirCluster, bool rootDirectory)
{
    List<FATEntryInfo> results;
    List<LFNEntry> lfnEntries;

    uint32_t sector = 0;    
    uint32_t cluster = dirCluster;

    while ((cluster != CLUSTER_FREE) && (cluster < CLUSTER_END))
    {
        /*
        With FAT12 and FAT16 the root directory is positioned after the File Allocation Table
        This is calculated below
        FAT32 does not use this technique
        */

        if(rootDirectory && this->FatType != FAT32 && sector == 0)
            sector = this->firstDataSector - this->rootDirSectors;
        
        else if(this->FatType == FAT32 || !rootDirectory)
            sector = ClusterToSector(cluster);

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
                
                if(entry->Attributes == ATTR_LONG_NAME) {   // Long file name entry
                    LFNEntry* lfn = (LFNEntry*)entry;       // Turn the directory entry into a LFNEntry using the magic of pointers
                    lfnEntries.push_back(*lfn);             // Add it to our buffer
                    continue;
                }

                // This is a valid entry, so add it to our list
                FATEntryInfo item;
                item.entry = *entry;
                item.sector = sector + i;
                item.offsetInSector = entryIndex * sizeof(DirectoryEntry);
                if(lfnEntries.size() > 0) { // We have some LFN entries in our list that belong to this entry
                    item.filename = ParseLFNEntries(&lfnEntries, *entry);
                    lfnEntries.Clear();
                }
                else
                    item.filename = ParseShortFilename((char*)entry->FileName);
                
                results.push_back(item);
            }
        }

        if(rootDirectory && this->FatType != FAT32) {
            //FAT_DEBUG("FAT Root directory has more than 16 entries, reading next sector", 0);
            sector++;
            continue; // No need to calculate the next cluster
        }

        cluster = ReadTable(cluster);
        //Log(Info, "Next cluster is %x", cluster);
    }
    Log(Error, "This should not be reached %s %d", __FILE__, __LINE__);
    return results;
}

FATEntryInfo* FAT::SeachInDirectory(char* name, uint32_t dirCluster, bool rootDirectory)
{
    List<FATEntryInfo> childs = GetDirectoryEntries(dirCluster, rootDirectory);
    FATEntryInfo* ret = 0;

    for(FATEntryInfo item : childs) {
        bool match = String::strcmp(name, item.filename);
        if(!ret && match) {
            ret = new FATEntryInfo();
            MemoryOperations::memcpy(ret, &item, sizeof(FATEntryInfo));
        }
        if(!match) // We can not delete the string that is returned as result
            delete item.filename;
    }

    childs.Clear();
    return ret;
}

FATEntryInfo* FAT::GetEntryByPath(char* path)
{
    uint32_t searchCluster = this->rootDirCluster;
    List<char*> pathList = String::Split(path, PATH_SEPERATOR_C);
    FATEntryInfo* ret = 0;

    // The path represents a entry in the root directory, for example just: "test.txt"
    if(pathList.size() == 0)
        return SeachInDirectory(path, searchCluster, true);
    
    // Loop through each part in the filename
    for(int i = 0; i < pathList.size(); i++)
    {
        FATEntryInfo* entry = SeachInDirectory(pathList[i], searchCluster, i == 0);
        if(entry == 0) { // Error while getting entry in directory
            ret = 0;
            goto end;
        }
        
        if(i == pathList.size() - 1) {
            ret = entry; // This is the last entry in the list, so this is the correct one
            goto end;
        }

        bool isDirectory = (entry->entry.Attributes & ATTR_DIRECTORY);
        if(isDirectory)
            searchCluster = GET_CLUSTER(entry->entry); // Search next sub-directory

        delete entry->filename;
        delete entry;
        
        if(!isDirectory) { // Item found is not a directory 
            ret = 0;
            goto end;
        }
    }
    // Entry not found
    ret = 0;

end:
    for(char* str : pathList)
        delete str;
    
    return ret;
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
char* FAT::ParseLFNEntries(List<LFNEntry>* entries, DirectoryEntry sfnEntry)
{
    // Calculate checksum of short file name
    uint8_t shortChecksum = Checksum((char*)sfnEntry.FileName);

    // Allocate space for complete name
    char* longName = new char[entries->size() * 13 + 1]; // Each LFN holds 13 characters + one for termination
    MemoryOperations::memset(longName, 0, entries->size() * 13 + 1);

    for(LFNEntry item : *entries)
    {
        if(item.checksum != shortChecksum) {
            Log(Error, "Checksum of LFN entry is incorrect");
            return longName;
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

#pragma region Write Support

bool FAT::FindEntryStartpoint(uint32_t cluster, uint32_t entryCount, bool rootDirectory, uint32_t* targetCluster, uint32_t* targetSector, uint32_t* sectorOffset)
{
    uint32_t sector = 0;
    uint32_t freeCount = 0;
    while ((cluster != CLUSTER_FREE) && (cluster < CLUSTER_END))
    {
        // For explanation check FAT::GetDirectoryEntries()
        if(rootDirectory && this->FatType != FAT32 && sector == 0)
            sector = this->firstDataSector - this->rootDirSectors;
        
        else if(this->FatType == FAT32 || !rootDirectory)
            sector = ClusterToSector(cluster);

        for(uint16_t i = 0; i < this->sectorsPerCluster; i++) // Loop through sectors in this cluster
        {
            if(this->disk->ReadSector(this->StartLBA + sector + i, this->readBuffer) != 0) {
                Log(Error, "Error reading disk at lba %d", this->StartLBA + sector + i);
                return false;
            }

            for(uint8_t entryIndex = 0; entryIndex < (this->bytesPerSector / sizeof(DirectoryEntry)); entryIndex++) // Loop through entries in this sector
            {
                DirectoryEntry* entry = (DirectoryEntry*)(this->readBuffer + entryIndex * sizeof(DirectoryEntry));

                if(entry->FileName[0] == ENTRY_END || entry->FileName[0] == ENTRY_UNUSED) // Unused or free entry
                {
                    if(freeCount == 0) { // First free entry
                        // Store start variables
                        *targetCluster = cluster;
                        *targetSector = i;
                        *sectorOffset = entryIndex * sizeof(DirectoryEntry);
                    }

                    freeCount++; // Increase the count of free entries
                    if(freeCount == entryCount) {   // We need exaclty this amount of free entries
                        return true;                // Variables should be filled in already
                    }
                }
                else {
                    freeCount = 0;        // Reset Counter
                    *targetCluster = 0;   // And Variables
                    *targetSector = 0;
                    *sectorOffset = 0;
                }
            }
        }

        if(rootDirectory && this->FatType != FAT32) {
            //FAT_DEBUG("FAT Root directory has more than 16 entries, reading next sector", 0);
            sector++;

            uint32_t rootDirLastSector = this->firstDataSector - this->rootDirSectors + (224 * sizeof(DirectoryEntry) / this->bytesPerSector);
            if(sector >= rootDirLastSector) {
                Log(Warning, "FAT Root directory has no more space for new entries");
                return false; // Let's hope this never happens
            }

            continue; // No need to calculate the next cluster
        }

        uint32_t next = ReadTable(cluster);             // Read next cluster in list
        if(next >= CLUSTER_END) {                       // No more clusters in this row
            uint32_t newCluster = AllocateCluster();    // Just allocate a new one for this directory
            ClearCluster(newCluster);                   // Empty cluster
            WriteTable(cluster, newCluster);            // Add to the clusterchain
            cluster = newCluster;
        }
        else
            cluster = next;
    }
    return false;
}

char* FAT::CreateShortFilename(char* name)
{
    char* result = new char[12];
    MemoryOperations::memset(result, ' ', 11);
    result[11] = '\0';

    int len = String::strlen(name);
    int dotIndex = String::IndexOf(name, '.');

    // Write the extension
    if(dotIndex >= 0) {
        for(int i = 0; i < 3; i++) {
            int charIndex = dotIndex + 1 + i;
            uint8_t c = charIndex >= len ? ' ' : String::Uppercase(name[charIndex]);
            result[8 + i] = c;
        }
    }
    // No extension in name
    else {
        for(int i = 0; i < 3; i++) {
            result[8 + i] = ' ';
        }
    }

    // Write the filename.
    int flen = len;
    if(dotIndex >= 0)
        flen = dotIndex;
    
    if(flen > 8) {
        // Write the name with the thingy
        for(int i = 0; i < 6; i++)
            result[i] = String::Uppercase(name[i]);

        result[6] = '~';
        result[7] = '1'; // Just assume there is only one version of this file
    }
    else {
        // Just write the file name.
        for(int i = 0; i < flen; i++) {
            result[i] = String::Uppercase(name[i]);
        }
    }

    return result;
}

List<LFNEntry> FAT::CreateLFNEntriesFromName(char* name, int num, uint8_t checksum)
{
    List<LFNEntry> entries;

    int charsWritten = 0;
    int nameLen = String::strlen(name);
    char* namePtr = name;

    for(int n = 0; n < num; n++)
    {
        LFNEntry entry;
        MemoryOperations::memset(&entry, 0, sizeof(LFNEntry));

        entry.entryIndex = n + 1;
        entry.checksum = checksum;
        entry.Attributes = ATTR_LONG_NAME;

        // First part of filename
        for(int i = 0; i < 9; i+=2) {
            if(charsWritten < nameLen)
                entry.namePart1[i] = *namePtr;
            else
                entry.namePart1[i] = 0;
            namePtr++;
            charsWritten++;
        }

        // Second part of filename
        for(int i = 0; i < 11; i+=2) {
            if(charsWritten < nameLen)
                entry.namePart2[i] = *namePtr;
            else
                entry.namePart2[i] = 0;
            namePtr++;
            charsWritten++;
        }

        // Third part of filename
        for(int i = 0; i < 3; i+=2) {
            if(charsWritten < nameLen)
                entry.namePart3[i] = *namePtr;
            else
                entry.namePart3[i] = 0;
            namePtr++;
            charsWritten++;
        }

        // Last entry in row
        if(n == num-1)
            entry.entryIndex |= LFN_ENTRY_END;
        
        entries.push_front(entry);
    }
    
    return entries;
}

bool FAT::WriteLongFilenameEntries(List<LFNEntry>* entries, uint32_t targetCluster, uint32_t targetSector, uint32_t sectorOffset, bool rootDirectory)
{
    uint32_t sector = 0;
    for(int i = 0; i < entries->size(); i++)
    {
        uint32_t entryOffset = sectorOffset + i * sizeof(DirectoryEntry);

        if(rootDirectory && this->FatType != FAT32 && sector == 0)
            sector = this->firstDataSector - this->rootDirSectors;
        
        else if(this->FatType == FAT32 || !rootDirectory)
            sector = ClusterToSector(targetCluster);
        
        if(this->disk->ReadSector(this->StartLBA + sector + targetSector, this->readBuffer) != 0)
            return false;

        // Copy entry to free spot
        LFNEntry target = entries->GetAt(i);
        MemoryOperations::memcpy(this->readBuffer + entryOffset, &target, sizeof(LFNEntry));

        // And copy back to the disk
        if(this->disk->WriteSector(this->StartLBA + sector + targetSector, this->readBuffer) != 0)
            return false;

        if(entryOffset + sizeof(DirectoryEntry) >= this->bytesPerSector) { // Check if we get outside of sector border for next write
            if(rootDirectory && this->FatType != FAT32) {
                sector++;           // Move onto next sector
                sectorOffset = 0;   // And reset offset
            }
            else if((this->FatType == FAT32) || (!rootDirectory)) {
                if(sector + 1 > this->sectorsPerCluster) // Outside cluster boundary
                    targetCluster = ReadTable(targetCluster);
                else
                    sector++;
            }
        }
    }
    return true;
}

bool FAT::WriteDirectoryEntry(DirectoryEntry entry, uint32_t targetSector, uint32_t sectorOffset, bool rootDirectory)
{
    // Read disk at sector
    if(this->disk->ReadSector(this->StartLBA + targetSector, this->readBuffer) != 0)
        return false;

    // Copy entry to free spot
    MemoryOperations::memcpy(this->readBuffer + sectorOffset, &entry, sizeof(DirectoryEntry));

    // And copy back to the disk
    if(this->disk->WriteSector(this->StartLBA + targetSector, this->readBuffer) != 0)
        return false;
    
    return true;
}

DirectoryEntry* FAT::CreateEntry(uint32_t parentCluster, char* name, uint8_t attr, bool rootDirectory, uint32_t targetCluster, uint32_t* sectorPlaced)
{
    // Length of complete name
    uint8_t nameLength = String::strlen(name);
    if(nameLength == 0)
        return 0;

    // How many LFN entries do we need?
    uint32_t requiredLFNEntries = nameLength / 13;
    if(nameLength % 13 > 0)
        requiredLFNEntries++;

    // How many Directory entries do we need in total? (Including LFN)
    uint32_t requiredEntries = requiredLFNEntries + 1;

    uint32_t entryCluster, entrySector, sectorOffset;
    if(FindEntryStartpoint(parentCluster, requiredEntries, rootDirectory, &entryCluster, &entrySector, &sectorOffset) == false)
        return 0;
    
    //FAT_DEBUG("Placing new chain of entries at %d:%d:%d", entryCluster, entrySector, sectorOffset);

    char* shortName = CreateShortFilename(name);
    List<LFNEntry> lfnEntries = CreateLFNEntriesFromName(name, requiredLFNEntries, Checksum(shortName));

    //FAT_DEBUG"Writing %d LFN Entries to disk", lfnEntries.size());
    if(WriteLongFilenameEntries(&lfnEntries, entryCluster, entrySector, sectorOffset, rootDirectory) == false) {
        delete shortName;
        return 0;
    }

    if(rootDirectory && this->FatType != FAT32)
    {
        entrySector = this->firstDataSector - this->rootDirSectors;
        sectorOffset += lfnEntries.size() * sizeof(LFNEntry);
        if(sectorOffset >= this->bytesPerSector) {
            entrySector++;
            sectorOffset = sectorOffset % this->bytesPerSector;
        }
    }
    else
    {
        sectorOffset += lfnEntries.size() * sizeof(LFNEntry);
        if(sectorOffset >= this->bytesPerSector) {
            entrySector++;
            sectorOffset = sectorOffset % this->bytesPerSector;

            if(entrySector >= this->sectorsPerCluster) {
                entryCluster = ReadTable(entryCluster);
                entrySector = 0;
                //sectorOffset = sectorOffset % this->bytesPerSector;

                if(entryCluster >= CLUSTER_END) {
                    uint32_t tmp = AllocateCluster();
                    ClearCluster(tmp);
                    WriteTable(entryCluster, tmp);
                    entryCluster = tmp;
                }
            }
        }

        entrySector = ClusterToSector(entryCluster) + entrySector;
    }

    // Create entry to write to the disk
    DirectoryEntry entry;
    MemoryOperations::memset(&entry, 0, sizeof(DirectoryEntry));

    // Copy name
    MemoryOperations::memcpy(entry.FileName, shortName, 11);

    // Set cluster
    entry.LowFirstCluster = targetCluster & 0xFFFF;
    entry.HighFirstCluster = (targetCluster >> 16) & 0xFFFF;

    // Set Attributes
    entry.Attributes = attr;

    // Set Date and Time
    entry.CreationDate = FatDate();
    entry.CreationTime = FatTime();
    entry.CreationTimeTenth = 100;
    entry.AccessDate = entry.CreationDate;
    entry.ModifyDate = entry.CreationDate;
    entry.ModifyTime = entry.CreationTime;

    // Finaly write it to the disk
    if(WriteDirectoryEntry(entry, entrySector, sectorOffset, rootDirectory) == false) {
        delete shortName;
        return 0;
    }
    
    // Free memory used by name creation
    delete shortName;

    // Set return variables and return
    if(sectorPlaced)
        *sectorPlaced = entrySector;
    
    return (DirectoryEntry*)(this->readBuffer + sectorOffset);
}

int FAT::CreateNewDirFileEntry(const char* path, uint8_t attributes)
{
    List<char*> pathParts = String::Split(path, PATH_SEPERATOR_C);
    DirectoryEntry* ret = 0;
    uint32_t entryRootCluster = 0; 
    if(pathParts.size() == 0)
    {
        uint32_t newCluster = AllocateCluster();        
        ret = CreateEntry(this->rootDirCluster, (char*)path, attributes, true, newCluster, 0);
    }
    else
    {
        char* parentDirectory = (char*)path;
        int i = String::IndexOf(path, PATH_SEPERATOR_C, pathParts.size() - 2);

        // Kinda stupid way to split filename and filepath, but it works!
        char tmp = parentDirectory[i];
        parentDirectory[i] = '\0';
        
        char* name = (char*)(path + i + 1);
        FATEntryInfo* parentEntry = GetEntryByPath(parentDirectory);

        // Place seperator back into filepath
        parentDirectory[i] = tmp;

        if(parentEntry != 0) 
        {
            uint32_t newCluster = AllocateCluster();
            ret = CreateEntry(GET_CLUSTER(parentEntry->entry), name, attributes, false, newCluster, 0);

            // Extract cluster of root directory for the .. entry
            entryRootCluster = GET_CLUSTER(parentEntry->entry);

            // Free memory
            delete parentEntry->filename;
            delete parentEntry;
        }
    }

    for(char* str : pathParts)
        delete str;

    if(ret != 0)
    {
        // Readbuffer gets trashed by ClearCluster so we need to make a copy.
        DirectoryEntry createdEntry = *ret;

        // Extract cluster from created entry, should be newCluster
        uint32_t directoryCluster = GET_CLUSTER((*ret));

        // Fat Documentation says that we should clear the cluster for a new directory/file
        ClearCluster(directoryCluster);

        // Also . and .. entries need to be created for a directory
        if(attributes & ATTR_DIRECTORY) {
            uint32_t sector = ClusterToSector(directoryCluster);
            
            // Create . entry
            DirectoryEntry dotEntry;
            MemoryOperations::memset(&dotEntry, 0, sizeof(DirectoryEntry));
            MemoryOperations::memcpy(dotEntry.FileName, ".          ", 11);
            dotEntry.Attributes = ATTR_DIRECTORY;

            // Use same date/time as new entry
            dotEntry.AccessDate = createdEntry.AccessDate;
            dotEntry.CreationDate = createdEntry.CreationDate;
            dotEntry.CreationTime = createdEntry.CreationTime;
            dotEntry.CreationTimeTenth = createdEntry.CreationTimeTenth;
            dotEntry.ModifyDate = createdEntry.ModifyDate;
            dotEntry.ModifyTime = createdEntry.ModifyTime;

            // Point entry to the main directory
            dotEntry.LowFirstCluster = directoryCluster & 0xFFFF;
            dotEntry.HighFirstCluster = (directoryCluster >> 16) & 0xFFFF;

            // Read first sector of new cluster, should be all zero's since we cleared it
            if(this->disk->ReadSector(this->StartLBA + sector, this->readBuffer) != 0)
                return -1;

            // Copy first . entry
            MemoryOperations::memcpy(this->readBuffer, &dotEntry, sizeof(DirectoryEntry));

            // Modify entry to point to parent
            dotEntry.LowFirstCluster = entryRootCluster & 0xFFFF;
            dotEntry.HighFirstCluster = (entryRootCluster >> 16) & 0xFFFF;
            MemoryOperations::memcpy(dotEntry.FileName, "..         ", 11); // And change name to the ..

            // Copy second .. entry
            MemoryOperations::memcpy(this->readBuffer + sizeof(DirectoryEntry), &dotEntry, sizeof(DirectoryEntry));

            // Finaly write it back to the disk
            if(this->disk->WriteSector(this->StartLBA + sector, this->readBuffer) != 0)
                return -1;
        }
        
        return 0;
    }
    
    return -1;
}

bool FAT::ModifyEntry(FATEntryInfo* entry, DirectoryEntry newVersion)
{
    if(entry == 0)
        return false;

    if(this->disk->ReadSector(this->StartLBA + entry->sector, this->readBuffer) != 0)
        return false;
    
    // Create pointer to entry for easy access
    DirectoryEntry* entryPtr = (DirectoryEntry*)(this->readBuffer + entry->offsetInSector);

    // Check if this is indeed the correct item, just to be sure
    if(MemoryOperations::memcmp(&entry->entry, entryPtr, sizeof(DirectoryEntry)) != 0)
        return false;
    
    // Overwrite old entry
    MemoryOperations::memcpy(entryPtr, &newVersion, sizeof(DirectoryEntry));

    // And finally write sector back to disk
    if(this->disk->WriteSector(this->StartLBA + entry->sector, this->readBuffer) != 0)
        return false;

    return true;
}
#pragma endregion


#pragma region DateTime
uint16_t FAT::FatTime()
{
    return (uint16_t)System::rtc->GetHour() << 11 | (uint16_t)System::rtc->GetMinute() << 5 | (uint16_t)System::rtc->GetSecond() >> 1;
}
uint16_t FAT::FatDate()
{
    return (uint16_t)(System::rtc->GetYear()-1980) << 9 | (uint16_t)System::rtc->GetMonth() << 5 | (uint16_t)System::rtc->GetDay();
}
#pragma endregion






List<LIBCactusOS::VFSEntry>* FAT::DirectoryList(const char* path)
{ 
    List<LIBCactusOS::VFSEntry>* ret = new List<LIBCactusOS::VFSEntry>();
    uint32_t parentCluster = this->rootDirCluster;
    bool rootdir = String::strlen(path) == 0;

    if(!rootdir) // Not the Root directory
    {
        FATEntryInfo* parent = GetEntryByPath((char*)path);
        parentCluster = GET_CLUSTER(parent->entry);
        
        delete parent->filename;
        delete parent;
    }

    List<FATEntryInfo> childs = GetDirectoryEntries(parentCluster, rootdir);
    for(FATEntryInfo item : childs) {
        // Create new entry and clear it to 0's
        LIBCactusOS::VFSEntry entry;
        MemoryOperations::memset(&entry, 0, sizeof(LIBCactusOS::VFSEntry));
        
        // Fill in the info
        entry.size = item.entry.FileSize;
        entry.isDir = item.entry.Attributes & ATTR_DIRECTORY;
        entry.creationDate.day = item.entry.CreationDate & 0b11111;
        entry.creationDate.month = (item.entry.CreationDate >> 5) & 0b1111;
        entry.creationDate.year = ((item.entry.CreationDate >> 9) & 0b1111111) + 1980;

        entry.creationTime.sec = (item.entry.CreationTime & 0b11111) * 2;
        entry.creationTime.min = (item.entry.CreationTime >> 5) & 0b111111;
        entry.creationTime.hour = (item.entry.CreationTime >> 11) & 0b11111;

        int len = String::strlen(item.filename);
        MemoryOperations::memcpy(entry.name, item.filename, len < VFS_NAME_LENGTH ? len : VFS_NAME_LENGTH);
        
        ret->push_back(entry);
    }

    return ret;
}

uint32_t FAT::GetFileSize(const char* path)
{
    FATEntryInfo* entry = GetEntryByPath((char*)path);
    uint32_t fileSize = 0;
    if(entry == 0)
        return 0;
    
    if(entry->entry.Attributes & ATTR_DIRECTORY)
       fileSize = -1; 
    else
        fileSize = entry->entry.FileSize;

    delete entry->filename;
    delete entry;

    return fileSize;
}
int FAT::ReadFile(const char* path, uint8_t* buffer, uint32_t offset, uint32_t len)
{ 
    if((int)len == -1)
        len = GetFileSize(path);

    FATEntryInfo* entry = GetEntryByPath((char*)path);
    if(entry == 0)
        return -1;
    
    if(entry->entry.Attributes & ATTR_DIRECTORY) {
        delete entry->filename;
        delete entry;
        return -1;
    }

    uint32_t cluster = GET_CLUSTER(entry->entry);
    uint8_t* bufferPointer = buffer;
    uint32_t bytesRead = 0;

    // Not needed anymore
    delete entry->filename;
    delete entry;
    
    while ((cluster != CLUSTER_FREE) && (cluster < CLUSTER_END))
    {
        uint32_t sector = ClusterToSector(cluster);

        for(uint16_t i = 0; i < this->sectorsPerCluster; i++) // Loop through sectors in this cluster
        {
            if(this->disk->ReadSector(this->StartLBA + sector + i, this->readBuffer) != 0) {
                Log(Error, "Error reading disk at lba %d", this->StartLBA + sector + i);
                return -1;
            }

            uint32_t remaingBytes = len - bytesRead;

            //Copy the required part of the buffer
            MemoryOperations::memcpy(bufferPointer, this->readBuffer, remaingBytes <= this->bytesPerSector ? remaingBytes : this->bytesPerSector);

            bytesRead += this->bytesPerSector;
            bufferPointer += this->bytesPerSector;
        }
        cluster = ReadTable(cluster);
    }

    return 0;
}

int FAT::WriteFile(const char* path, uint8_t* buffer, uint32_t len, bool create)
{
    if(FileExists(path) == false && create)
        if(CreateFile(path) != 0)
            return -1;

    // Get entry
    FATEntryInfo* entry = GetEntryByPath((char*)path);
    if(entry == 0)
        return -1;

    uint32_t reqClusters = len / this->clusterSize;
    if(len % this->clusterSize != 0)
        reqClusters++;

    uint32_t bytesWritten = 0;

    // File should have first cluster allocated already
    uint32_t cluster = GET_CLUSTER(entry->entry);
    for(uint32_t i = 0; i < reqClusters; i++) {
        uint32_t sector = ClusterToSector(cluster);
        for(uint32_t s = 0; s < this->sectorsPerCluster; s++) {
            uint32_t bytesLeft = len - bytesWritten;

            // Use readbuffer for partial writing when there is not a complete sector left
            if(bytesLeft < this->bytesPerSector) {
                MemoryOperations::memset(this->readBuffer, 0, this->bytesPerSector);        
                MemoryOperations::memcpy(this->readBuffer, buffer + i*this->clusterSize + s*this->bytesPerSector, bytesLeft > this->bytesPerSector ? this->bytesPerSector : bytesLeft);

                // Write sector with data to the disk
                if(this->disk->WriteSector(this->StartLBA + sector + s, this->readBuffer) != 0) {
                    delete entry->filename;
                    delete entry;
                    return -1;
                }
            }
            else // Much faster routine for complete sectors (Much might be a overstatement, specialy for floppies)
            {
                if(this->disk->WriteSector(this->StartLBA + sector + s, buffer + i*this->clusterSize + s*this->bytesPerSector) != 0) {
                    delete entry->filename;
                    delete entry;
                    return -1;
                }
            }

            // And update variables
            bytesWritten += this->bytesPerSector;
        }
        
        // No need to clear cluster, will get overwritten
        uint32_t newCluster = AllocateCluster();
        WriteTable(cluster, newCluster);
        cluster = newCluster;
    }

    // Now we need to modify some variables in the entry
    DirectoryEntry newEntry = entry->entry;
    newEntry.FileSize = len;
    newEntry.ModifyDate = FatDate();
    newEntry.ModifyTime = FatTime();
    
    // Modify entry
    if(ModifyEntry(entry, newEntry) == false) {
        delete entry->filename;
        delete entry;
        return -1;
    }

    delete entry->filename;
    delete entry;
    return 0;
}

int FAT::CreateFile(const char* path)
{
    return CreateNewDirFileEntry(path, 0);
}

int FAT::CreateDirectory(const char* path)
{
    return CreateNewDirFileEntry(path, ATTR_DIRECTORY);
}

bool FAT::FileExists(const char* path)
{
    FATEntryInfo* entry = GetEntryByPath((char*)path);
    bool exists = false;
    if(entry == 0)
        return false;
    
    exists = !(entry->entry.Attributes & ATTR_DIRECTORY);

    delete entry->filename;
    delete entry;

    return exists;
}
bool FAT::DirectoryExists(const char* path)
{
    FATEntryInfo* entry = GetEntryByPath((char*)path);
    bool exists = false;
    if(entry == 0)
        return false;
    
    exists = (entry->entry.Attributes & ATTR_DIRECTORY);

    delete entry->filename;
    delete entry;

    return exists;
}