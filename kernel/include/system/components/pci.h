#ifndef __CACTUSOS__SYSTEM__COMPONENTS__PCI_H
#define __CACTUSOS__SYSTEM__COMPONENTS__PCI_H

#include <core/port.h>
#include <common/list.h>
#include <system/components/systemcomponent.h>
#include <system/bootconsole.h>
#include <common/printf.h>
#include <system/initrd.h>

namespace CactusOS
{
    namespace system
    {
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
            CactusOS::common::uint8_t* address;
            CactusOS::common::uint32_t size;
            BaseAddressRegisterType type;
        } __attribute__((packed));

        class PCIController : public SystemComponent
        {
        private:
            bool DeviceHasFunctions(common::uint16_t bus, common::uint16_t device);
            char* GetClassCodeString(common::uint8_t classID, common::uint8_t subClassID);
        protected:
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