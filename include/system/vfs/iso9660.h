#ifndef __CACTUSOS__SYSTEM__VFS__ISO9660_H
#define __CACTUSOS__SYSTEM__VFS__ISO9660_H

#include <common/types.h>

#include <system/vfs/virtualfilesystem.h>

namespace CactusOS
{
    namespace system
    {
        struct DirectoryRecord
        {
            
        } __attribute__((packed));

        struct VolumeDescriptor
        {
            common::uint8_t Type;
            char Identifier[5];
            common::uint8_t version;
            char data[2041];
        } __attribute__((packed));

        struct PrimaryVolumeDescriptor
        {
            common::uint8_t Type;
            char Identifier[5];
            common::uint8_t version;
            common::uint8_t reserved1;
            char SystemIdentifier[32];
            char VolumeIdentifier[32];

            
        } __attribute__((packed));

        #define ISO_START_SECTOR 0x10
        #define CDROM_SECTOR_SIZE 2048

        enum VolumeDescriptorType
        {
            BootRecord = 0,
            PVDescriptor = 1,
            SupplementaryVolumeDescriptor = 2,
            VolumePartitionDescriptor = 3,
            VolumeDescriptorSetTerminator = 255
        };


        class ISO9660 : public VirtualFileSystem
        {
        public:
            ISO9660(Disk* disk, common::uint32_t start, common::uint32_t size);

            bool Initialize();
        };
    }
}

#endif