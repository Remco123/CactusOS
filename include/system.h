#ifndef __CACTUSOS__SYSTEM_H
#define __CACTUSOS__SYSTEM_H

#include <common/types.h>

#include <core/gdt.h>
#include <core/interrupts.h>
#include <core/memorymanagement.h>
#include <core/pit.h>
#include <core/port.h>
#include <core/rtc.h>
#include <core/cpu.h>
#include <core/pci.h>
#include <core/smbios.h>

#include <multiboot/multiboot.h>

namespace CactusOS
{
    struct Core
    {
        core::GlobalDescriptorTable* gdt;
        core::InterruptManager* interrupts;
        core::MemoryManager* memoryManager;
        core::PeripheralComponentInterconnectController* pci;
        core::PIT* pit;
        core::CPU* cpu;
    };

    class System
    {
    public:
        static Core* core;

        static void InitCore();
    };
}

#endif