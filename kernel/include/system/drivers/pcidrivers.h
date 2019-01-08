#ifndef __CACTUSOS__SYSTEM__DRIVERS__PCIDRIVERS_H
#define __CACTUSOS__SYSTEM__DRIVERS__PCIDRIVERS_H

#include <system/components/pci.h>
#include <system/drivers/drivermanager.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            struct PCIAttachedDriverEntry
            {
                common::uint16_t vendorID;
                common::uint16_t deviceID;
                char* driverString;
            } __attribute__((packed));

            class PCIDrivers
            {
            public:
                static void AssignDriversFromPCI(PCIController* pci, DriverManager* driverManager);
            };
        }
    }
}


#endif