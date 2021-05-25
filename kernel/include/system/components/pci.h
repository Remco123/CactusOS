#ifndef __CACTUSOS__SYSTEM__COMPONENTS__PCI_H
#define __CACTUSOS__SYSTEM__COMPONENTS__PCI_H

#include <core/port.h>
#include <common/list.h>
#include <system/components/systemcomponent.h>
#include <system/bootconsole.h>
#include <system/initrd.h>

namespace CactusOS
{
    namespace system
    {
        #define PCI_CMDREG_IO   (1<<0)
        #define PCI_CMDREG_MEM  (1<<1)
        #define PCI_CMDREG_BM   (1<<2)

        struct PCIDevice
        {
            common::uint8_t interrupt;
            common::uint32_t portBase;
            
            common::uint16_t bus;
            common::uint16_t device;
            common::uint16_t function;

            common::uint16_t deviceID;
            common::uint16_t vendorID;

            common::uint8_t classID;
            common::uint8_t subclassID;
            common::uint8_t programmingInterfaceID;
            common::uint8_t revisionID;
        } __attribute__((packed));

        enum BaseAddressRegisterType
        {
            MemoryMapping = 0,
            InputOutput = 1
        };

        struct BaseAddressRegister
        {
            bool prefetchable;
            BaseAddressRegisterType type;
            CactusOS::common::uint32_t size;
            CactusOS::common::uint64_t address;
        };

        class PCIController : public SystemComponent
        {
        private:
            bool DeviceHasFunctions(common::uint16_t bus, common::uint16_t device);
            char* GetClassCodeString(common::uint8_t classID, common::uint8_t subClassID);
        public:
            common::uint32_t Read(common::uint16_t bus, common::uint16_t device, common::uint16_t function, common::uint32_t registeroffset);
            void Write(common::uint16_t bus, common::uint16_t device, common::uint16_t function, common::uint32_t registeroffset, common::uint32_t value);
            BaseAddressRegister GetBaseAddressRegister(common::uint16_t bus, common::uint16_t device, common::uint16_t function, common::uint16_t bar);
        public:
            List<PCIDevice*> deviceList;

            PCIController();

            void PopulateDeviceList();
        };
    }
}

#endif