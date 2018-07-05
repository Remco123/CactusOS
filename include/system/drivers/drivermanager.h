#ifndef __CACTUSOS__SYSTEM__DRIVERMANAGER__DRIVER_H
#define __CACTUSOS__SYSTEM__DRIVERMANAGER__DRIVER_H

#include <system/drivers/networkdriver.h>
#include <common/types.h>
#include <core/pci.h>
#include <core/interrupts.h>

#include <system/drivers/rtl8139.h>
#include <system/drivers/amd_am79c973.h>

namespace CactusOS
{
    namespace system
    {
        class DriverManager
        {
        public:
            DriverManager();
            ~DriverManager();

            void AddDriver(Driver* driver);
            //Find drivers for the connected pci devices
            void AssignDrivers(core::PeripheralComponentInterconnectController* pciController, core::InterruptManager* interrupts, core::PeripheralComponentInterconnectController* pci);
            Driver* GetDriver(core::PeripheralComponentInterconnectDeviceDescriptor* device, core::InterruptManager* interrupts, core::PeripheralComponentInterconnectController* pci);
            void ActivateAll();

            Driver* DriverByType(DriverType type);
        };
    }
}

#endif