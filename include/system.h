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

#include <system/drivers/drivermanager.h>

#include <multiboot/multiboot.h>

namespace CactusOS
{
    struct sys_Core
    {
        core::GlobalDescriptorTable* gdt;
        core::InterruptManager* interrupts;
        core::MemoryManager* memoryManager;
        core::PeripheralComponentInterconnectController* pci;
        core::PIT* pit;
        core::CPU* cpu;
    };

    struct sys_System
    {
        CactusOS::system::DriverManager* driverManager;
    };

    class System
    {
    public:
        static sys_Core* core;
        static sys_System* system;

        static void InitCore();
        static void InitSystem();
    };
}

#endif