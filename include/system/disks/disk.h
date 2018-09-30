#ifndef __CACTUSOS__SYSTEM__DISKS__DISK_H
#define __CACTUSOS__SYSTEM__DISKS__DISK_H

#include <common/types.h>

#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        class DiskController;

        class Disk
        {
        private:
            common::uint32_t diskNumber; //The real number for the disk
            DiskController* controller;
        public:
            Disk(common::uint32_t number, DiskController* controller);
            
            char ReadSector(common::uint32_t lba, common::uint8_t* buf);
            
            char WriteSector(common::uint32_t lba, common::uint8_t* buf);
        };
    }
}

#endif