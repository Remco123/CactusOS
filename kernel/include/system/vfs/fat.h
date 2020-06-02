#ifndef __CACTUSOS__SYSTEM__VFS__FAT32_H
#define __CACTUSOS__SYSTEM__VFS__FAT32_H

#include <system/vfs/virtualfilesystem.h>

namespace CactusOS
{
    namespace system
    {
        struct FAT32_BPB
        {
            common::uint8_t     bootCode[3];
            common::uint8_t     Oem_Id[8];
            common::uint16_t    bytesPerSector;
            common::uint8_t     SectorsPerCluster;
            common::uint16_t    ReservedSectors;
            common::uint8_t     NumOfFats;
            common::uint16_t    NumDirEntries;
            common::uint16_t    TotalSectorsSmall;
            common::uint8_t     MediaDescriptorType;
            common::uint16_t    SectorsPerFat12_16;
            common::uint16_t    SectorsPerTrack;
            common::uint16_t    NumHeads;
            common::uint32_t    HiddenSectors;
            common::uint32_t    TotalSectorsBig;

            //Beginning of fat32 values
            common::uint32_t    SectorsPerFat32;
            common::uint16_t    Flags;
            common::uint16_t    FATVersionNum;
            common::uint32_t    RootDirCluster;
            common::uint16_t    FSInfoSector;
            common::uint16_t    BackupBootSector;
            common::uint8_t     Reserved[12];
            common::uint8_t     DriveNum;
            common::uint8_t     WinNTFlags;
            common::uint8_t     Signature;
            common::uint32_t    VolumeIDSerial;
            common::uint8_t     VolumeLabel[11];
            common::uint8_t     SystemIDString[8];
            common::uint8_t     BootCode[420];
            common::uint16_t    BootSignature;
        } __attribute__((packed));

        struct FAT32_FSInfo
        {
            common::uint32_t    signature1;
            common::uint8_t     reserved1[480];
            common::uint32_t    signature2;
            common::uint32_t    lastFreeCluster;
            common::uint32_t    startSearchCluster;
            common::uint8_t     reserved2[12];
            common::uint32_t    signature3;
        } __attribute__((packed));

        struct DirectoryEntry
        {
            common::uint8_t     FileName[11];
            common::uint8_t     Attributes;
            common::uint8_t     Reserved;
            common::uint8_t     CTimeTenthSecs;
            common::uint16_t    CTime;
            common::uint16_t    CDate;
            common::uint16_t    ADate;
            common::uint16_t    HighFirstCluster;
            common::uint16_t    MTime;
            common::uint16_t    MDate;
            common::uint16_t    LowFirstCluster;
            common::uint32_t    FileSize;
        } __attribute__((packed));

        // Cluster Special Values
        #define CLUSTER_END_32  0x0FFFFFF8
        #define CLUSTER_BAD_32  0x0FFFFFF7
        #define CLUSTER_FREE_32 0x00000000
        
        #define CLUSTER_END_16  0xFFF8
        #define CLUSTER_BAD_16  0xFFF7
        #define CLUSTER_FREE_16 0x0000
        
        #define CLUSTER_END_12  0xFF8
        #define CLUSTER_BAD_12  0xFF7
        #define CLUSTER_FREE_12 0x000

        #define ATTR_READ_ONLY  0x01
        #define ATTR_HIDDEN 	0x02
        #define ATTR_SYSTEM     0x04
        #define ATTR_VOLUME_ID  0x08
        #define ATTR_DIRECTORY	0x10
        #define ATTR_ARCHIVE    0x20
        #define ATTR_LONG_NAME 	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

        enum FATType
        {
            FAT12,
            FAT16,
            FAT32
        };
        
        class FAT : public VirtualFileSystem
        {
        private: // Variables
            FATType FatType;                    // What type of filesystem is this?
            char* FatTypeString = 0;            // String describing the filesystem type

            uint16_t bytesPerSector = 0;        // Bytes per sector, usually 512
            uint32_t rootDirSectors = 0;        // How many sectors does the root directory use?
            uint8_t sectorsPerCluster = 0;      // How many sectors does one cluster contain?
            uint32_t clusterSize = 0;           // Size of one cluster in bytes

            uint32_t firstDataSector = 0;       // LBA Address of first sector of the data region
            uint32_t firstFatSector = 0;        // The first sector in the File Allocation Table
            uint32_t rootDirCluster = 0;        // Cluster of the root directory
            uint32_t totalClusters = 0;         // Total amount of clusters used by data region

            uint8_t* readBuffer = 0;            // Buffer used for reading the disk
            
        private: // Helper functions
            // Convert a cluster number to its corresponding start sector
            common::uint32_t ClusterToSector(common::uint32_t cluster);

            // Reads the FAT table and returns the value for the specific cluster
            common::uint32_t ReadTable(common::uint32_t cluster);

            // Writes a new value into the FAT table for the cluster
            void WriteTable(common::uint32_t cluster, common::uint32_t value);

            // Allocate a new cluster in the FAT Table
            common::uint32_t AllocateCluster();
        public:
            FAT(Disk* disk, common::uint32_t start, common::uint32_t size);

            bool Initialize();

            //////////////
            // VFS Implementations
            //////////////

            // Read file contents into buffer
            int ReadFile(const char* filename, uint8_t* buffer, uint32_t offset = 0, uint32_t len = -1);
            // Write buffer to file, file will be created when create equals true
            int WriteFile(const char* filename, uint8_t* buffer, uint32_t len, bool create = true);

            // Check if file exist
            bool FileExists(const char* filename);
            // Check if directory exist
            bool DirectoryExists(const char* filename);

            // Create a file at the filepath
            int CreateFile(const char* path);
            // Create a new directory
            int CreateDirectory(const char* path);

            // Get size of specified file in bytes
            uint32_t GetFileSize(const char* filename);
            // Returns list of context inside a directory
            List<char*>* DirectoryList(const char* path);
        };
    }
}

#endif