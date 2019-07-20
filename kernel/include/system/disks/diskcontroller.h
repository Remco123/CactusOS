#ifndef __CACTUSOS__SYSTEM__DISKS__DISKCONTROLLER_H
#define __CACTUSOS__SYSTEM__DISKS__DISKCONTROLLER_H


#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>

#include <system/disks/disk.h>
#include <system/disks/diskmanager.h>

namespace CactusOS
{
    namespace system
    {
        class DiskController
        {       
        public:
            DiskController();

            virtual char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            virtual char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);   
            virtual bool EjectDrive(common::uint8_t drive);         
        };
    }
}


#endif