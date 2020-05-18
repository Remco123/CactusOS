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
        
        typedef union {
            struct {
                uint16_t day : 5;  // Day of month, 1 - 31
                uint16_t mon : 4;   // Month, 1 - 12
                uint16_t year : 7;  // Year, counting from 1980.
            };
            uint16_t intValue;
        } FAT_DATE;

        typedef union {
            struct {
                uint16_t sec : 5;   // Seconds divided by 2.
                uint16_t min : 6;   // Minutes, 0 - 59
                uint16_t hour : 5;  // Hour, 0 - 23
            };
            uint16_t intValue;
        } FAT_TIME;

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

        #define FAT_READ_ONLY   0x01
        #define FAT_HIDDEN      0x02
        #define FAT_SYSTEM      0x04
        #define FAT_VOLUME_ID   0x08
        #define FAT_DIRECTORY   0x10
        #define FAT_ARCHIVE     0x20
        #define FAT_LFN (FAT_READ_ONLY|FAT_HIDDEN|FAT_SYSTEM|FAT_VOLUME_ID)

        #define FAT_ENDOFDIRS       0x0
        #define FAT_UNUSED          0xE5
        #define FAT_FILENAME_LEN    11

        #define FAT_CLUSTER_END         0x0FFFFFF8
        #define FAT_CLUSTER_BAD         0x0FFFFFF7
        #define FAT_CLUSTER_FREE        0x00000000
        
        #define GET_CLUSTER(e) (e->LowFirstCluster | (e->HighFirstCluster << (16)))
        #define CLUSTER_TO_SECTOR(x) (((x - 2) * sectorsPerCluster) + firstDataSector)

        class FAT32 : public VirtualFileSystem
        {
        private: // Variables
            common::uint32_t bytesPerSector = 0;
            common::uint32_t totalClusters = 0;
            common::uint32_t sectorsPerCluster = 0;
            common::uint32_t firstFatSector = 0;
            common::uint32_t firstDataSector = 0;
            common::uint32_t reservedSectors = 0;
            common::uint32_t rootCluster;

            uint8_t readBuffer[512];        
        private: // Functions
            
            // Table Functions
            common::uint32_t ReadFATTable(common::uint32_t cluster);
            int WriteFATTable(common::uint32_t cluster, common::uint32_t value);
            common::uint32_t AllocateNewCluster();

            // Internal directory parsing functions
            List<DirectoryEntry> ReadDir(common::uint32_t cluster);
            List<common::uint32_t> GetClusterChain(common::uint32_t firstcluster, common::uint64_t* numclus);
            
            // Internal search functions
            DirectoryEntry SearchInDirectory(common::uint32_t cluster, const char* name);
            DirectoryEntry GetEntry(const char* path);

            // Filename convertion
            char* ToFatFormat(char* str);
            char* ToNormalFormat(char* str);

            // Date and time functions
            FAT_DATE CurrentDate();
            FAT_TIME CurrentTime();

            // Create a new FAT entry of type type
            int CreateEntry(const char* path, char type);
        public:
            FAT32(Disk* disk, common::uint32_t start, common::uint32_t size);

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