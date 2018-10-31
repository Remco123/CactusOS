#ifndef __CACTUSOS__CORE__GDT_H
#define __CACTUSOS__CORE__GDT_H

#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        class GlobalDescriptorTable
        {
            public:

                class SegmentDescriptor
                {
                    private:
                        CactusOS::common::uint16_t limit_lo;
                        CactusOS::common::uint16_t base_lo;
                        CactusOS::common::uint8_t base_hi;
                        CactusOS::common::uint8_t type;
                        CactusOS::common::uint8_t limit_hi;
                        CactusOS::common::uint8_t base_vhi;

                    public:
                        SegmentDescriptor(CactusOS::common::uint32_t base, CactusOS::common::uint32_t limit, CactusOS::common::uint8_t type);
                        CactusOS::common::uint32_t Base();
                        CactusOS::common::uint32_t Limit();
                } __attribute__((packed));

            private:
                SegmentDescriptor nullSegmentSelector;
                SegmentDescriptor codeSegmentSelector;
                SegmentDescriptor dataSegmentSelector;
                SegmentDescriptor userCodeSegmentSelector;
                SegmentDescriptor userDataSegmentSelector;

            public:
                SegmentDescriptor tssSegmentSelector;

                GlobalDescriptorTable();
                ~GlobalDescriptorTable();

                CactusOS::common::uint16_t CodeSegmentSelector();
                CactusOS::common::uint16_t DataSegmentSelector();
                CactusOS::common::uint16_t UserCodeSegmentSelector();
                CactusOS::common::uint16_t UserDataSegmentSelector();
        };
    }
}
    
#endif