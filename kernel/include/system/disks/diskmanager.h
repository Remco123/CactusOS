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
        struct BiosDriveParameters
        {
            common::uint16_t bufLen;
            common::uint16_t infoFlags;
            common::uint32_t cilinders;
            common::uint32_t heads;
            common::uint32_t sectorsPerTrack;
            common::uint64_t totalSectors;
            common::uint16_t bytesPerSector;

            common::uint32_t eddParameters;

            common::uint16_t signature;
            common::uint8_t  devPathLen;
            common::uint8_t  reserved1[3];
            char             hostBusName[4];
            char             interfaceName[8];
            common::uint8_t  interfacePath[8];
            common::uint8_t  devicePath[8];
            common::uint8_t  reserved2;
            common::uint8_t  checksum;
        } __attribute__((packed));

        class Disk;
        class DiskManager
        {
        public:
            List<Disk*> allDisks;

            DiskManager();

            //Add disk to system and check for filesystems
            void AddDisk(Disk* disk);
            //Remove disk from system and unmount filesystems
            void RemoveDisk(Disk* disk);

            char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);

            BiosDriveParameters* GetDriveInfoBios(common::uint8_t drive);
        };
    }
}


#endif