#ifndef __CACTUSOS__SYSTEM__VFS__FAT_H
#define __CACTUSOS__SYSTEM__VFS__FAT_H

#include <common/types.h>
#include <core/memorymanagement.h>

#include <system/vfs/virtualfilesystem.h>

namespace CactusOS
{
    namespace system
    {
        struct FATExtendedBootSector32
        {
            unsigned int		table_size_32;
            unsigned short		extended_flags;
            unsigned short		fat_version;
            unsigned int		root_cluster;
            unsigned short		fat_info;
            unsigned short		backup_BS_sector;
            unsigned char 		reserved_0[12];
            unsigned char		drive_number;
            unsigned char 		reserved_1;
            unsigned char		boot_signature;
            unsigned int 		volume_id;
            unsigned char		volume_label[11];
            unsigned char		fat_type_label[8];
        } __attribute__((packed));

        struct FATExtendedBootSector12_16
        {
            unsigned char		bios_drive_num;
            unsigned char		reserved1;
            unsigned char		boot_signature;
            unsigned int		volume_id;
            unsigned char		volume_label[11];
            unsigned char		fat_type_label[8];
        } __attribute__((packed));

        struct FATBootSector
        {
            unsigned char 		bootjmp[3];
            unsigned char 		oem_name[8];
            unsigned short 	    bytes_per_sector;
            unsigned char		sectors_per_cluster;
            unsigned short		reserved_sector_count;
            unsigned char		table_count;
            unsigned short		root_entry_count;
            unsigned short		total_sectors_16;
            unsigned char		media_type;
            unsigned short		table_size_16;
            unsigned short		sectors_per_track;
            unsigned short		head_side_count;
            unsigned int 		hidden_sector_count;
            unsigned int 		total_sectors_32;
        
            //this will be cast to it's specific type once the driver actually knows what type of FAT this is.
            unsigned char		extended_section[54];
        } __attribute__((packed));

        enum FATType
        {
            FAT12,
            FAT16,
            FAT32,
            ExFAT
        };

        #define FAT_SECTOR_SIZE 512

        class FATFileSystem : public VirtualFileSystem
        {
        private:
            unsigned int first_fat_sector;
            unsigned int first_data_sector;
            unsigned int total_clusters;
            FATType fat_type;
        public:
            FATFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size);

            bool Initialize();
        };
    }
}

#endif