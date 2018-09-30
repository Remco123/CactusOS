#ifndef __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H
#define __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H


#include <common/types.h>
#include <common/convert.h>
#include <common/memoryoperations.h>

#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        class DiskManager
        {
        private:
            DiskController* controllers[32];
            common::uint32_t numControllers;

        public:
            
        };
    }
}


#endif