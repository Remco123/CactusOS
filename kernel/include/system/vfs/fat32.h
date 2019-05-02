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

        #define FAT_READ_ONLY 0x01
        #define FAT_HIDDEN 0x02
        #define FAT_SYSTEM 0x04
        #define FAT_VOLUME_ID 0x08
        #define FAT_DIRECTORY 0x10
        #define FAT_ARCHIVE 0x20
        #define FAT_LFN (FAT_READ_ONLY|FAT_HIDDEN|FAT_SYSTEM|FAT_VOLUME_ID)

        #define FAT_ENDOFDIRS 0x0
        #define FAT_UNUSED 0xE5
        #define FAT_FILENAME_LEN 11
        
        #define GET_CLUSTER(e) (e->LowFirstCluster | (e->HighFirstCluster << (16)))

        class FAT32 : public VirtualFileSystem
        {
        private:
            common::uint32_t bytesPerSector = 0;
            common::uint32_t totalClusters = 0;
            common::uint32_t sectorsPerCluster = 0;
            common::uint32_t firstFatSector = 0;
            common::uint32_t firstDataSector = 0;
            common::uint32_t reservedSectors = 0;
            common::uint32_t rootCluster;

            common::uint32_t ClusterToSector(common::uint32_t cluster);
            List<DirectoryEntry>* ReadDir(common::uint32_t cluster);
            List<common::uint32_t>* GetClusterChain(common::uint32_t firstcluster, common::uint64_t* numclus);
            
            DirectoryEntry SearchInDirectory(DirectoryEntry* searchIn, const char* name);
            DirectoryEntry SearchInDirectory(common::uint32_t cluster, const char* name);
            DirectoryEntry GetEntry(const char* path);

            char* ToFatFormat(char* str);
            char* ToNormalFormat(char* str);
        public:
            FAT32(Disk* disk, common::uint32_t start, common::uint32_t size);

            bool Initialize();

            //////////////
            // VFS Implementations
            //////////////
            List<char*>* DirectoryList(char* path);
            int GetFileSize(char* path);
            int ReadFile(char* path, common::uint8_t* buffer);
            bool FileExists(char* path);
            bool DirectoryExists(char* path);
        };
    }
}

#endif