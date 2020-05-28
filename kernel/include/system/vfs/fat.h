#ifndef __CACTUSOS__SYSTEM__VFS__FAT32_H
#define __CACTUSOS__SYSTEM__VFS__FAT32_H

#include <system/vfs/virtualfilesystem.h>
#include <../src/system/vfs/FatFS/ff.h>

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

        #define FAT_VOLUME_ID   0x08
        #define FAT_CLUSTER_END         0x0FFFFFF8
        
        class FAT : public VirtualFileSystem
        {
        private:
            FATFS baseFS;
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