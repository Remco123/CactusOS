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
            Floppy,
            CD
        };

        class Disk
        {
        private:
            common::uint32_t diskNumber; //The real number for the disk
            DiskController* controller;
        public:
            DiskType type;
            Disk(common::uint32_t number, DiskController* controller, DiskType type);
            
            char ReadSector(common::uint32_t lba, common::uint8_t* buf);
            
            char WriteSector(common::uint32_t lba, common::uint8_t* buf);
        };
    }
}

#endif