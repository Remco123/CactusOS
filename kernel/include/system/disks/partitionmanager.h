#ifndef __CACTUSOS__SYSTEM__DISKS__PARTITIONMANAGER_H
#define __CACTUSOS__SYSTEM__DISKS__PARTITIONMANAGER_H

#include <system/disks/diskmanager.h>
#include <system/vfs/vfsmanager.h>

namespace CactusOS
{
    namespace system
    {
        struct PartitionTableEntry
        {
            common::uint8_t bootable;

            common::uint8_t start_head;
            common::uint8_t start_sector : 6;
            common::uint16_t start_cylinder : 10;

            common::uint8_t partition_id;

            common::uint8_t end_head;
            common::uint8_t end_sector : 6;
            common::uint16_t end_cylinder : 10;
            
            common::uint32_t start_lba;
            common::uint32_t length;
        } __attribute__((packed));
        

        struct MasterBootRecord
        {
            common::uint8_t bootloader[440];
            common::uint32_t signature;
            common::uint16_t unused;
            
            PartitionTableEntry primaryPartitions[4];
            
            common::uint16_t magicnumber;
        } __attribute__((packed));

        class PartitionManager
        {
        private:
            //Check partition type and assign filesystem driver if available
            static void AssignVFS(PartitionTableEntry partition, Disk* disk);
        public:
            //Read partition descriptor of disk and assign fileysysem if possible
            static void DetectAndLoadFilesystem(Disk* disk);
        };
    }
}

#endif