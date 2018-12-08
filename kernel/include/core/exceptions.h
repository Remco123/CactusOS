#ifndef __CACTUSOS__CORE__EXCEPTIONS_H
#define __CACTUSOS__CORE__EXCEPTIONS_H

#include <common/types.h>
#include <system/bootconsole.h>
#include <common/convert.h>
#include <core/idt.h>
#include <common/printf.h>

namespace CactusOS
{
    namespace core
    {
        class Exceptions
        {
        private:
            static common::uint32_t DivideByZero(common::uint32_t esp);
            static common::uint32_t GeneralProtectionFault(common::uint32_t esp);
            static common::uint32_t PageFault(common::uint32_t esp);
        public:
            static common::uint32_t HandleException(common::uint32_t number, common::uint32_t esp);
        };
    }
}

#endif