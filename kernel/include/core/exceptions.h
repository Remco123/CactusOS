#ifndef __CACTUSOS__CORE__EXCEPTIONS_H
#define __CACTUSOS__CORE__EXCEPTIONS_H

#include <common/types.h>
#include <system/bootconsole.h>
#include <common/convert.h>
#include <core/idt.h>
#include <common/print.h>

namespace CactusOS
{
    namespace core
    {
        struct SelectorErrorCode
        {
            common::uint8_t External : 1;
            common::uint8_t Table : 2;
            common::uint16_t TableIndex : 13;
        } __attribute__((packed));
        

        class Exceptions
        {
        private:
            static common::uint32_t DivideByZero(common::uint32_t esp);
            static common::uint32_t GeneralProtectionFault(common::uint32_t esp);
            static common::uint32_t PageFault(common::uint32_t esp);
            static common::uint32_t TrapException(common::uint32_t esp);
        public:
            static common::uint32_t HandleException(common::uint32_t number, common::uint32_t esp);
        };
    }
}

#endif