#ifndef __CACTUSOS__CORE__SMBIOS_H
#define __CACTUSOS__CORE__SMBIOS_H

#include <common/types.h>
#include <common/convert.h>

namespace CactusOS
{
    namespace core
    {
        struct SMBIOSEntryPoint
        {
            char EntryPointString[4];    //This is _SM_
            common::uint8_t Checksum;              //This value summed with all the values of the table, should be 0 (overflow)
            common::uint8_t Length;                //Length of the Entry Point Table. Since version 2.1 of SMBIOS, this is 0x1F
            common::uint8_t MajorVersion;          //Major Version of SMBIOS
            common::uint8_t MinorVersion;          //Minor Version of SMBIOS
            common::uint16_t MaxStructureSize;     //Maximum size of a SMBIOS Structure (we will se later)
            common::uint8_t EntryPointRevision;    //...
            char FormattedArea[5];       //...
            char EntryPointString2[5];   //This is _DMI_
            common::uint8_t Checksum2;             //Checksum for values from EntryPointString2 to the end of table
            common::uint16_t TableLength;          //Length of the Table containing all the structures
            common::uint32_t TableAddress;	     //Address of the Table
            common::uint16_t NumberOfStructures;   //Number of structures in the table
            common::uint8_t BCDRevision;           //Unused
        } __attribute__((packed));

        struct SMBIOSTag
        {
            common::uint8_t type;
            common::uint8_t length;
            common::uint16_t handle;
        };

        struct SMBIOSBIOSInfo
        {
            common::uint8_t type;
            common::uint8_t length;
            common::uint16_t handle;
            common::uint8_t vendor;
            common::uint8_t version;
            common::uint16_t start_addr_seg;
            common::uint8_t release_date;
            common::uint8_t rom_size;
            common::uint64_t characteristics;
            common::uint8_t extra_char_1;
            common::uint8_t extra_char_2;
            common::uint8_t major_release;
            common::uint8_t minor_release;
            common::uint8_t controller_major_release;
            common::uint8_t controller_minor_release;
            common::uint16_t ebios_size;
        } __attribute__((packed));

        struct SMBIOSCPUInfo
        {
            common::uint8_t type;
            common::uint8_t length;
            common::uint16_t handle;
            common::uint8_t socket;
            common::uint8_t cpu_type;
            common::uint8_t family;
            common::uint8_t manufacturer;
            common::uint64_t id;
            common::uint8_t version;
            common::uint8_t voltage;
            common::uint16_t ext_clock;
            common::uint16_t max_speed;
            common::uint16_t curr_speed;
            common::uint8_t status;
        } __attribute__((packed));

        class SMBIOS
        {
        private:
            static SMBIOSEntryPoint* entry;
            static const char* get_string(common::uint8_t *offset, common::uint8_t number);
            static common::uint32_t* find_strings_end(common::uint8_t *offset);
        public:
            static SMBIOSEntryPoint* Find();
            static SMBIOSBIOSInfo* BiosInfo();
            static SMBIOSCPUInfo* CpuInfo();
        };
    }
}

#endif