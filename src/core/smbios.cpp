#include <core/smbios.h>

using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);
void printfHex32(uint32_t);

SMBIOSEntryPoint* SMBIOS::entry = 0;

const char* SMBIOS::get_string(uint8_t *offset, uint8_t number)
{
    uint8_t idx = 1;
    while (offset[0] != '\0' || offset[1] != '\0')
    {
        if (idx == number) { return (const char*)(offset); }

        if (offset[0] == '\0') ++idx;
        ++offset;
    }

    return 0;
}


uint32_t* SMBIOS::find_strings_end(uint8_t *offset)
{
    while (offset[0] != '\0' || offset[1] != '\0')
    {
        ++offset;
    }
    return ((uint32_t*)(offset)) + 2;
}


SMBIOSEntryPoint* SMBIOS::Find()
{
    char *mem = (char *) 0xF0000;
    int length, i;
    unsigned char checksum;
    while ((unsigned int) mem < 0x100000) {
        if (mem[0] == '_' && mem[1] == 'S' && mem[2] == 'M' && mem[3] == '_') {
            length = mem[5];
            checksum = 0;
            for(i = 0; i < length; i++) {
                checksum += mem[i];
            }
            if(checksum == 0) break;
        }
        mem += 16;
    }

    if ((unsigned int) mem == 0x100000) {
        printf("No SMBIOS found!");
        return 0;
    }
    else
    {
        printf("SMBIOS found at : "); printfHex32((uint32_t)mem); printf("\n");
        entry = (SMBIOSEntryPoint*)(mem);
        printf(" SMBIOS version : "); printf(Convert::IntToString(entry->MajorVersion)); printf("."); printf(Convert::IntToString(entry->MinorVersion)); printf("\n");
        printf(" SMBIOS entries : "); printf(Convert::IntToString(entry->NumberOfStructures)); printf("\n");
        return entry;
    }
}


SMBIOSBIOSInfo* SMBIOS::BiosInfo()
{
    if (entry)
    {
        uint32_t* mem = (uint32_t*)(entry->TableAddress);
        while (mem < (uint32_t*)(entry->TableAddress) + entry->TableLength)
        {
            SMBIOSTag* tag = (SMBIOSTag*)(mem);
            if (tag->type == 127)
            {
                break;
            }
            if (tag->type == 0)
            {
                SMBIOSBIOSInfo* info = (SMBIOSBIOSInfo*)(mem);

                uint8_t* tag_end = (uint8_t*)(mem + tag->length - 1);

                printf(" BIOS Vendor : "); printf((char*)get_string(tag_end, info->vendor)); printf("\n");
                printf(" BIOS Version : "); printf((char*)get_string(tag_end, info->version)); printf("\n");
                printf(" BIOS Release date : "); printf((char*)get_string(tag_end, info->release_date)); printf("\n");

                return info;
            }

            mem += tag->length - 1;
            mem = find_strings_end((uint8_t*)(mem));
        }

        return 0;
    }
    else
    {
        printf("SMBIOS Was not found!");
        return 0;
    }
}


SMBIOSCPUInfo* SMBIOS::CpuInfo()
{
    if (entry)
    {
        uint32_t* mem = (uint32_t*)(entry->TableAddress);
        while (mem < (uint32_t*)(entry->TableAddress) + entry->TableLength)
        {
            SMBIOSTag* tag = (SMBIOSTag*)(mem);
            if (tag->type == 127)
            {
                break;
            }
            if (tag->type == 4)
            {
                SMBIOSCPUInfo* info = (SMBIOSCPUInfo*)(mem);

                uint8_t* tag_end = (uint8_t*)(mem + tag->length - 1);

                printf(" Processor Type : ");
                switch (info->cpu_type)
                {
                case 0:
                case 1:
                case 2:
                default:
                    printf("Unknown\n");
                    break;
                case 3:
                    printf("CPU\n");
                    break;
                case 4:
                    printf("Math processor\n");
                    break;
                case 5:
                    printf("DSP processor\n");
                    break;
                case 6:
                    printf("Video processor\n");
                    break;
                }

                //log(" Processor family : %s\n", dmi_processor_family(info->family));
                printf(" Processor socket : "); printf((char*)get_string(tag_end, info->socket+1)); printf("\n");
                printf(" Processor manufacturer : "); printf((char*)get_string(tag_end, info->manufacturer+1)); printf("\n");
                printf(" Processor version : "); printf((char*)get_string(tag_end, info->version+1)); printf("\n");
                printf(" Processor voltage : ");
                if ((info->voltage >> 7) & 1)
                {
                    printf(Convert::IntToString((info->voltage & 0b1111111)/10.0)); printf(".1fv\n");
                }
                else
                {
                    printf(Convert::IntToString(info->voltage == 0 ? 5 : info->voltage == 1 ? 3.3 : info->voltage == 2 ? 2.9 : -1)); printf(".1fv\n");
                }

                printf(" Processor speed : "); printf(Convert::IntToString(info->curr_speed)); printf(" MHz\n");

                return info;
            }

            mem += tag->length - 1;
            mem = find_strings_end((uint8_t*)(mem));
        }
        printf("No BIOS CPU Info was found!\n");
        return 0;
    }
    printf("SMBIOS Was not found!\n");
    return 0;
}