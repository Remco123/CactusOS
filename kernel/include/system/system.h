#ifndef __CACTUSOS__SYSTEM__SYSTEM_H
#define __CACTUSOS__SYSTEM__SYSTEM_H

#include <system/bootconsole.h>
#include <system/components/systemcomponent.h>
#include <system/components/pit.h>
#include <system/components/rtc.h>
#include <system/components/smbios.h>
#include <system/components/vesa.h>
#include <system/components/pci.h>
#include <system/virtual8086/VM86Manager.h>
#include <system/virtual8086/VM86Monitor.h>

#include <system/initrd.h>
#include <system/drivers/drivermanager.h>
#include <system/drivers/pcidrivers.h>

namespace CactusOS
{
    namespace system
    {
        class System
        {
        public:
            static multiboot_info_t* mbi;
            static PIT* pit;
            static RTC* rtc;
            static SMBIOS* smbios;
            static Virtual8086Manager* vm86Manager;
            static Virtual8086Monitor* vm86Monitor;
            static VESA* vesa;
            static PCIController* pci;
            static drivers::DriverManager* driverManager;

            static void Start();
        };
    }
} 

#endif