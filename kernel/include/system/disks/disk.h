#ifndef __CACTUSOS__SYSTEM__DISKS__DISK_H
#define __CACTUSOS__SYSTEM__DISKS__DISK_H

#include <common/types.h>

#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        class DiskController;

        enum DiskType
        {
            HardDisk,
            USBDisk,
            Floppy,
            CDROM
        };

        class Disk
        {
        public:
            DiskController* controller;         // Which controller is controling this disk device
            common::uint32_t controllerIndex;   // The real number for the disk on the controller
            char* identifier = 0;               // Disk Identifier
            DiskType type;              // Type of disk
            common::uint64_t size;      // Size of disk in bytes
            common::uint32_t numBlocks; // Number of data blocks
            common::uint32_t blockSize; // Size of one block of data

            Disk(common::uint32_t controllerIndex, DiskController* controller, DiskType type, common::uint64_t size, common::uint32_t blocks, common::uint32_t blocksize);
            
            virtual char ReadSector(common::uint32_t lba, common::uint8_t* buf);          
            virtual char WriteSector(common::uint32_t lba, common::uint8_t* buf);
        };
    }
}

#endif