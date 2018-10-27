#include <core/pci.h>

using namespace CactusOS::common;
using namespace CactusOS::core;




PeripheralComponentInterconnectDeviceDescriptor::PeripheralComponentInterconnectDeviceDescriptor()
{
}

PeripheralComponentInterconnectDeviceDescriptor::~PeripheralComponentInterconnectDeviceDescriptor()
{
}







PeripheralComponentInterconnectController::PeripheralComponentInterconnectController()
{
    this->NumDevices = 0;
}

PeripheralComponentInterconnectController::~PeripheralComponentInterconnectController()
{
}

uint32_t PeripheralComponentInterconnectController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset)
{
    uint32_t id =
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        | (registeroffset & 0xFC);
    outportl(0xCF8, id);
    uint32_t result = inportl(0xCFC);
    return result >> (8* (registeroffset % 4));
}

void PeripheralComponentInterconnectController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value)
{
    uint32_t id =
        0x1 << 31
        | ((bus & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((function & 0x07) << 8)
        | (registeroffset & 0xFC);
    outportl(0xCF8, id);
    outportl(0xCFC, value); 
}

bool PeripheralComponentInterconnectController::DeviceHasFunctions(common::uint16_t bus, common::uint16_t device)
{
    return Read(bus, device, 0, 0x0E) & (1<<7);
}


void printf(char*);
void printfHex(uint8_t);

void PeripheralComponentInterconnectController::FindDevices()
{
    for(int bus = 0; bus < 256; bus++)
    {
        for(int device = 0; device < 32; device++)
        {
            int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for(int function = 0; function < numFunctions; function++)
            {
                PeripheralComponentInterconnectDeviceDescriptor* dev = GetDeviceDescriptor(bus, device, function);
                
                if(dev->vendor_id == 0x0000 || dev->vendor_id == 0xFFFF)
                {
                    delete dev;
                    continue;
                }
                
                
                for(int barNum = 0; barNum < 6; barNum++)
                {
                    BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
                    if(bar.address && (bar.type == InputOutput))
                        dev->portBase = (uint32_t)bar.address;
                }

                printf("Found device on: bus: ");
                printf(Convert::IntToString(bus));
                printf(" dev: ");
                printf(Convert::IntToString(device));
                printf(" func: ");
                printf(Convert::IntToString(function));
                printf("\n");

                this->Devices[NumDevices] = dev;
                this->NumDevices++;
                
            }
        }
    }
}


BaseAddressRegister PeripheralComponentInterconnectController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar)
{
    BaseAddressRegister result;
    
    
    uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
    int maxBARs = 6 - (4*headertype);
    if(bar >= maxBARs)
        return result;
    
    uint32_t bar_value = Read(bus, device, function, 0x10 + 4*bar);
    result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;    
    
    if(result.type == MemoryMapping)
    {
        
        switch((bar_value >> 1) & 0x3)
        {
            
            case 0: // 32 Bit Mode
            case 1: // 20 Bit Mode
            case 2: // 64 Bit Mode
                break;
        }
        
    }
    else // InputOutput
    {
        result.address = (uint8_t*)(bar_value & ~0x3);
        result.prefetchable = false;
    }
    
    
    return result;
}



PeripheralComponentInterconnectDeviceDescriptor* PeripheralComponentInterconnectController::GetDeviceDescriptor(uint16_t bus, uint16_t device, uint16_t function)
{
    PeripheralComponentInterconnectDeviceDescriptor* result = new PeripheralComponentInterconnectDeviceDescriptor();
    
    result->bus = bus;
    result->device = device;
    result->function = function;
    
    result->vendor_id = Read(bus, device, function, 0x00);
    result->device_id = Read(bus, device, function, 0x02);

    result->class_id = Read(bus, device, function, 0x0b);
    result->subclass_id = Read(bus, device, function, 0x0a);
    result->interface_id = Read(bus, device, function, 0x09);

    result->revision = Read(bus, device, function, 0x08);
    result->interrupt = Read(bus, device, function, 0x3c);
    
    return result;
}

/*
uint32_t PeripheralComponentInterconnectController::pci_read(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset) {
    uint32_t reg = 0x80000000;
    reg |= (bus & 0xFF) << 16;
    reg |= (device & 0x1F) << 11;
    reg |= (function & 0x7) << 8;
    reg |= (offset & 0xFF) & 0xFC;
    outportl(PCI_CONFIG, reg );
    return inportl(PCI_DATA);
}
void PeripheralComponentInterconnectController::pci_write(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint32_t data) {
    uint32_t reg = 0x80000000;
    reg |= (bus & 0xFF) << 16;
    reg |= (device & 0x1F) << 11;
    reg |= (function & 0x7) << 8;
    reg |= offset & 0xFC;
    outportl(PCI_CONFIG, reg );
    outportl(PCI_DATA, data);
}
*/