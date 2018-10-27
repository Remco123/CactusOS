#ifndef __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H
#define __CACTUSOS__SYSTEM__DISKS__DISKMANAGER_H


#include <common/types.h>
#include <common/convert.h>
#include <common/string.h>
#include <common/memoryoperations.h>

#include <core/interrupts.h>
#include <core/pit.h>
#include <system/disks/disk.h>

namespace CactusOS
{
    namespace system
    {
        class DiskController;
        class Disk;

        class DiskManager
        {
        private:
            DiskController* controllers[32]; //32 should be more than enough.
            common::uint32_t numControllers;
            core::PIT* pit;

        public:
            Disk* allDisks[32];
            common::uint32_t numDisks;

            DiskManager();
            
            void DetectAndLoadDisks(core::InterruptManager* interrupts, core::PIT* pit);
            void EjectDrive(common::uint32_t driveNumber);
            void BenchmarkDrive(common::uint32_t driveNumber);
        };
    }
}


#endif