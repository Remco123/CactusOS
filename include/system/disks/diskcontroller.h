#ifndef __CACTUSOS__SYSTEM__DISKS__DISKCONTROLLER_H
#define __CACTUSOS__SYSTEM__DISKS__DISKCONTROLLER_H


#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>

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
        };
    }
}


#endif