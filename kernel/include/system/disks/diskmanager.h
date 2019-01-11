#ifndef __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H
#define __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H


#include <common/types.h>
#include <common/convert.h>
#include <common/string.h>
#include <common/memoryoperations.h>
#include <common/list.h>

#include <system/bootconsole.h>
#include <system/disks/disk.h>

namespace CactusOS
{
    namespace system
    {
        class Disk;

        class DiskManager
        {
        public:
            List<Disk*> allDisks;

            DiskManager();

            void AddDisk(Disk* disk);

            char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
        };
    }
}


#endif