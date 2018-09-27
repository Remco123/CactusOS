#ifndef __CACTUSOS__SYSTEM__DISKS__DISKTYPE_H
#define __CACTUSOS__SYSTEM__DISKS__DISKTYPE_H


#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>

namespace CactusOS
{
    namespace system
    {
        enum DiskType
        {
            HardDisk,
            CDROM,
            Floppy
        };
    }
}


#endif