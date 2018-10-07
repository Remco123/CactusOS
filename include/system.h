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
#include <system/disks/diskmanager.h>
#include <system/disks/partitionmanager.h>

#include <multiboot/multiboot.h>

namespace CactusOS
{
    class System
    {
    public:
    //Variables
        static bool NetworkAvailible;
    //Core
        static core::GlobalDescriptorTable* gdt;
        static core::InterruptManager* interrupts;
        static core::MemoryManager* memoryManager;
        static core::PeripheralComponentInterconnectController* pci;
        static core::PIT* pit;
        static core::CPU* cpu;

    //System
        static CactusOS::system::DriverManager* driverManager;
        static CactusOS::system::NetworkManager* networkManager;
        static CactusOS::system::DiskManager* diskManager;

        static void InitCore();
        static void InitSystem();
    };
}

#endif