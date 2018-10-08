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
            common::uint8_t length;
            common::uint8_t ear_length;
            common::uint32_t extent_location;
            common::uint32_t extent_location_be; // big-endian, unused
            common::uint32_t data_length;
            common::uint32_t data_length_be; // big-endian, unused
            common::uint8_t datetime[7];
            common::uint8_t flags;
            common::uint8_t unit_size;
            common::uint8_t gap_size;
            common::uint16_t vol_seq_number;
            common::uint16_t vol_seq_number_be; // big-endian, unused
            common::uint8_t name_length;
            char name[0];
        } __attribute__((packed));

        struct PVDTimeFormat
        {
            char Year[4];
            char Month[2];
            char Day[2];
            char Hour[2];
            char Minute[2];
            char Second[2];
            char HundrdSecond[2];
            common::int8_t TimeZone;
        } __attribute__((packed));

        struct VolumeDescriptor
        {
            common::uint8_t Type;
            char Identifier[5];
            common::uint8_t version;
            char data[2041];
        } __attribute__((packed));

        #define ISODCL(from, to) (to - from + 1)

        struct PrimaryVolumeDescriptor
        {
            common::uint8_t type;
            char id                             [ISODCL (  2,     6)];
            common::uint8_t version;
            char reserved1                      [ISODCL (  8,     8)];
            char system_id                      [ISODCL (  9,    40)]; /* achars */
            char volume_id                      [ISODCL ( 41,    72)]; /* dchars */
            char reserved2                      [ISODCL ( 73,    80)];
            common::uint64_t volume_space_size;
            char reserved3                      [ISODCL ( 89,   120)];
            common::uint32_t volume_set_size;
            common::uint32_t volume_sequence_number;
            common::uint32_t logical_block_size;
            common::uint64_t path_table_size;
            common::uint32_t type_1_path_table;
            common::uint32_t opt_type_1_path_table;
            common::uint32_t type_m_path_table;
            common::uint32_t opt_type_m_path_table;
            DirectoryRecord root_directory_record;
            char volume_set_id                  [ISODCL (191,   318)]; /* dchars */
            char publisher_id                   [ISODCL (319,   446)]; /* achars */
            char preparer_id                    [ISODCL (447,   574)]; /* achars */
            char application_id                 [ISODCL (575,   702)]; /* achars */
            char copyright_file_id              [ISODCL (703,   739)]; /* dchars */
            char abstract_file_id               [ISODCL (740,   776)]; /* dchars */
            char bibliographic_file_id          [ISODCL (777,   813)]; /* dchars */
            PVDTimeFormat creation_date         ;
            PVDTimeFormat modification_date     ;
            PVDTimeFormat expiration_date       ;
            PVDTimeFormat effective_date        ;
            char file_structure_version         [ISODCL (882,   882)];
            char reserved4                      [ISODCL (883,   883)];
            common::uint8_t application_data    [ISODCL (884,  1395)];
            char reserved5                      [ISODCL (1396,  2048)];            
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