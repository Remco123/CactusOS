#ifndef __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MANAGER_H
#define __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86MANAGER_H

#include <system/interruptmanager.h>
#include <system/virtual8086/VM86Args.h>
#include <core/registers.h>
#include <core/idt.h>
#include <core/tss.h>

namespace CactusOS
{
    namespace system
    {
        class Virtual8086Manager : public InterruptHandler
        {
        private:
            void vm86Enter(common::uint16_t ss, common::uint16_t sp, common::uint16_t cs, common::uint16_t ip, common::uint32_t arg);
        public:
            Virtual8086Manager();
            common::uint32_t HandleInterrupt(common::uint32_t esp);
            
            // Call a bios interrupt while passing multiple arguments via the regs variable
            void CallInterrupt(common::uint8_t intNumber, VM86Arguments* regs);
            
            // Execute a specific function defined in VM8086Code.asm
            // Sometimes things can't be done using the CallInterrupt function
            void ExecuteCode(common::uint32_t instructionStart, common::uint32_t args);
        };
    }
}

#endif