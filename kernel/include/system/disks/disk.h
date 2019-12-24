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
        public:
            DiskController* controller; //Which controller is controling this disk device
            common::uint32_t controllerIndex; //The real number for the disk on the controller
            char* identifier = 0; //Disk Identifier

            Disk(common::uint32_t controllerIndex, DiskController* controller);
            
            virtual char ReadSector(common::uint32_t lba, common::uint8_t* buf);          
            virtual char WriteSector(common::uint32_t lba, common::uint8_t* buf);
        };
    }
}

#endif