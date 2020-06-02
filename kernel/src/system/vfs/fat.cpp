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
    for(int i = 0; i < 5; i++)
        Log(Info, "%d", AllocateCluster());
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
	uint32_t freeCluster = this->FatType == FAT12 ? CLUSTER_FREE_12 : (this->FatType == FAT16 ? CLUSTER_FREE_16 : CLUSTER_FREE_32);
	uint32_t endCluster = this->FatType == FAT12 ? CLUSTER_END_12 : (this->FatType == FAT16 ? CLUSTER_END_16 : CLUSTER_END_32);

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