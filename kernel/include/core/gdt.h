#ifndef __CACTUSOS__CORE__GDT_H
#define __CACTUSOS__CORE__GDT_H

#include <common/types.h>

using namespace CactusOS::common;

namespace CactusOS
{
    namespace core
    {
        struct GDTEntry
        {
            common::uint16_t limit_low;           // The lower 16 bits of the limit.
            common::uint16_t base_low;            // The lower 16 bits of the base.
            common::uint8_t  base_middle;         // The next 8 bits of the base.
            common::uint8_t  access;              // Access flags, determine what ring this segment can be used in.
            common::uint8_t  granularity;
            common::uint8_t  base_high;           // The last 8 bits of the base.
        } __attribute__((packed));

        struct GDTPointer
        {
            common::uint16_t limit;               // The upper 16 bits of all selector limits.
            common::uint32_t base;                // The address of the first gdt_entry_t struct.
        } __attribute__((packed));

        class GlobalDescriptorTable
        {        
        public:
            static void SetDescriptor(int number, common::uint32_t base, common::uint32_t limit, common::uint8_t access, common::uint8_t gran);
            static GDTEntry* GetDescriptor(int number);
            static void Init();
        };
    }
}

#endif