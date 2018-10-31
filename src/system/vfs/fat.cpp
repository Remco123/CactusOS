#include <system/vfs/fat.h>

using namespace CactusOS; 
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

FATFileSystem::FATFileSystem(Disk* disk, common::uint32_t start, common::uint32_t size)
: VirtualFileSystem(disk, start, size)
{
    this->ReadOnly = false;
    this->FilesystemName = "FAT(32/16/12) Filesystem";
}

bool FATFileSystem::Initialize()
{
    printf("Starting FAT fileystem\n");
    uint8_t* data = new uint8_t[FAT_SECTOR_SIZE];
    if(this->disk->ReadSector(this->StartLBA, data) == 0)
    {
        FATBootSector* bootSector = (FATBootSector*)data;
        printf("#### FAT boot sector ####\n");
        printf("# Bytes per sector: "); printf(Convert::IntToString(bootSector->bytes_per_sector)); printf("\n");
        printf("# OEM Name: "); printf((char*)bootSector->oem_name); printf("\n");
        printf("# Sectors per cluster: "); printf(Convert::IntToString(bootSector->sectors_per_cluster)); printf("\n");
        printf("# Number of FATS: "); printf(Convert::IntToString(bootSector->table_count)); printf("\n");
        printf("#########################\n");

        total_clusters = bootSector->total_sectors_16 / bootSector->sectors_per_cluster;
        if(total_clusters == 0) //The amount of clusters is in the 32 bit field
        {
            total_clusters = bootSector->total_sectors_32 / bootSector->sectors_per_cluster;
        }

        //Determine type of FAT
        if (total_clusters < 4085)
	    {
		    fat_type = FAT12;
            printf("FAT Type: FAT12\n");
		    first_data_sector = bootSector->reserved_sector_count + bootSector->table_count * bootSector->table_size_16 + (bootSector->root_entry_count * 32 + bootSector->bytes_per_sector - 1) / bootSector->bytes_per_sector;
        }
        else
        {
            if (total_clusters < 65525)
            {
                fat_type = FAT16;
                printf("FAT Type: FAT16\n");
                first_data_sector = bootSector->reserved_sector_count + bootSector->table_count * bootSector->table_size_16 + (bootSector->root_entry_count * 32 + bootSector->bytes_per_sector - 1) / bootSector->bytes_per_sector; 
            }
            else
            {
                fat_type = FAT32;
                printf("FAT Type: FAT32\n");
                first_data_sector = bootSector->reserved_sector_count + bootSector->table_count * ((FATExtendedBootSector32*)(bootSector->extended_section))->table_size_32;
            }
        }
        printf("First data sector: "); printf(Convert::IntToString(first_data_sector)); printf("\n");
        return false; //FAT is really not supported yet, we will stick to iso9660 for a while
    }
    else
    {
        printf("Error reading sector: "); printf(Convert::IntToString(this->StartLBA)); printf("\n");
        return false;
    }
}