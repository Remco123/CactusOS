#ifndef __CACTUSOS__HARDWARECOMMUNICATION__PCI_H
#define __CACTUSOS__HARDWARECOMMUNICATION__PCI_H

#include <core/port.h>
#include <common/types.h>
#include <common/convert.h>
#include <core/interrupts.h>

#include <core/memorymanagement.h>

namespace CactusOS
{
    namespace core
    {

        enum BaseAddressRegisterType
        {
            MemoryMapping = 0,
            InputOutput = 1
        };
        
        
        
        class BaseAddressRegister
        {
        public:
            bool prefetchable;
            CactusOS::common::uint8_t* address;
            CactusOS::common::uint32_t size;
            BaseAddressRegisterType type;
        };
        
        
        
        class PeripheralComponentInterconnectDeviceDescriptor
        {
        public:
            CactusOS::common::uint32_t portBase;
            CactusOS::common::uint32_t interrupt;
            
            CactusOS::common::uint16_t bus;
            CactusOS::common::uint16_t device;
            CactusOS::common::uint16_t function;

            CactusOS::common::uint16_t vendor_id;
            CactusOS::common::uint16_t device_id;
            
            CactusOS::common::uint8_t class_id;
            CactusOS::common::uint8_t subclass_id;
            CactusOS::common::uint8_t interface_id;

            CactusOS::common::uint8_t revision;
            
            PeripheralComponentInterconnectDeviceDescriptor();
            ~PeripheralComponentInterconnectDeviceDescriptor();
            
        };


        class PeripheralComponentInterconnectController
        {      
        public:
            PeripheralComponentInterconnectDeviceDescriptor* Devices[32];
            common::uint32_t NumDevices = 0;      

            PeripheralComponentInterconnectController();
            ~PeripheralComponentInterconnectController();
            
            CactusOS::common::uint32_t Read(CactusOS::common::uint16_t bus, CactusOS::common::uint16_t device, CactusOS::common::uint16_t function, CactusOS::common::uint32_t registeroffset);
            void Write(CactusOS::common::uint16_t bus, CactusOS::common::uint16_t device, CactusOS::common::uint16_t function, CactusOS::common::uint32_t registeroffset, CactusOS::common::uint32_t value);
            bool DeviceHasFunctions(CactusOS::common::uint16_t bus, CactusOS::common::uint16_t device);
            
            void FindDevices();
            PeripheralComponentInterconnectDeviceDescriptor* GetDeviceDescriptor(CactusOS::common::uint16_t bus, CactusOS::common::uint16_t device, CactusOS::common::uint16_t function);
            BaseAddressRegister GetBaseAddressRegister(CactusOS::common::uint16_t bus, CactusOS::common::uint16_t device, CactusOS::common::uint16_t function, CactusOS::common::uint16_t bar);
        };
    }
}
    
#endif