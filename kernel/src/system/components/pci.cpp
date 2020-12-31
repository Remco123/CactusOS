#include <system/components/pci.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

bool PCIController::DeviceHasFunctions(uint16_t bus, uint16_t device)
{
    return Read(bus, device, 0, 0x0E) & (1<<7);
}

uint32_t PCIController::Read(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset)
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

void PCIController::Write(uint16_t bus, uint16_t device, uint16_t function, uint32_t registeroffset, uint32_t value)
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


PCIController::PCIController()
: SystemComponent("PCI", "Peripheral Component Interconnect")
{
    this->deviceList.Clear();
}


void PCIController::PopulateDeviceList()
{
    Log(Info, "Scanning for PCI Devices");

    for(int bus = 0; bus < 256; bus++)
    {
        for(int device = 0; device < 32; device++)
        {
            int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
            for(int function = 0; function < numFunctions; function++)
            {
                uint16_t vendorID = Read(bus, device, function, 0x00);
                if(vendorID == 0xFFFF)
                    continue;

                // Add pci device to list
                PCIDevice* pciDevice = new PCIDevice();
                pciDevice->bus = bus;
                pciDevice->device = device;
                pciDevice->function = function;

                pciDevice->vendorID = vendorID;
                pciDevice->deviceID = Read(bus, device, function, 0x02);

                pciDevice->classID = Read(bus, device, function, 0x0B);
                pciDevice->subclassID = Read(bus, device, function, 0x0A);
                pciDevice->programmingInterfaceID = Read(bus, device, function, 0x09); 
                pciDevice->revisionID = Read(bus, device, function, 0x08);
                pciDevice->interrupt = Read(bus, device, function, 0x3C);

                // Get the portBase from the base address registers
                for(int barNum = 0; barNum < 6; barNum++)
                {
                    BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
                    if(bar.address && (bar.type == InputOutput))
                        pciDevice->portBase = (uint32_t)bar.address;
                }

                // Read status byte from device
                uint16_t status = (Read(bus, device, function, 0x04) & 0xFFFF0000) >> 16;
                if(status & (1<<4)) { // There is a capabilities list
                    uint8_t offset = Read(bus, device, function, 0x34) & 0xFF;
                    offset &= ~(0b11);
                    
                    while(offset) {
                        // Read value stored at offset, this contains ID, next offset and a feature specific uint16_t.
                        uint32_t capValue1 = Read(bus, device, function, offset);

                        // Extract ID from value read above
                        uint8_t id = capValue1 & 0xFF;

                        if(id == 1) // Power Management, see https://lekensteyn.nl/files/docs/PCI_Power_Management_12.pdf for details
                        {
                            // Read next 4 bytes of structure
                            uint32_t capValue2 = Read(bus, device, function, offset + 4);

                            //Log(Info, "CAP1 = %x    CAP2 = %x", capValue1, capValue2);

                            // Extract PMCSR from value
                            uint16_t powerValue = (capValue2 & 0xFFFF);
                            
                            // Powerstate is not D0 (Complete on)
                            if((powerValue & 0b11) != 0) {
                                powerValue &= ~(0b11); // Set it to D0
                                powerValue |= (1<<15); // Also set PME Status bit (not sure why)
                                Write(bus, device, function, offset + 4, (capValue2 & 0xFFFF0000) | powerValue);

                                System::pit->Sleep(10);

                                // Read second 4 bytes again to check if power on succeeded
                                capValue2 = Read(bus, device, function, offset + 4);

                                if((capValue2 & 0b11) != 0) // Power on failed
                                    Log(Error, "Could not enable power for device %d:%d:%d", bus, device, function);
                            }
                        }

                        offset = (capValue1 & 0xFF00) >> 8;
                    } 
                }

                Log(Info, "%d:%d:%d %w:%w %s", bus, device, function, pciDevice->vendorID, pciDevice->deviceID, GetClassCodeString(pciDevice->classID, pciDevice->subclassID));
                
                deviceList.push_back(pciDevice);
            }
        }
    }
    BootConsole::Write("Found a total of: "); BootConsole::Write(Convert::IntToString(deviceList.size())); BootConsole::WriteLine(" Devices");
}

char* PCIController::GetClassCodeString(uint8_t classID, uint8_t subClassID)
{
    char* idString = "??:??"; 
    char* classIDString = Convert::IntToHexString(classID);
    char* subClassIDString = Convert::IntToHexString(subClassID);

    idString[0] = classIDString[0];
    idString[1] = classIDString[1];

    idString[3] = subClassIDString[0];
    idString[4] = subClassIDString[1];

    uint32_t lookupTableSize = 0;
    char* lookupTable = (char*)InitialRamDisk::ReadFile("/PCI Class Codes.txt", &lookupTableSize);
    if(lookupTable == 0 || lookupTableSize == 0) {
        delete classIDString;
        delete subClassIDString;
        return "Database Error";
    }

    uint32_t tableIndex = 0;
    while(tableIndex < lookupTableSize)
    {
        uint32_t strLength = 0;
        char* refString = lookupTable + tableIndex;
        while(*refString != '\n')
        {
            strLength++;
            refString++;
        }
        if(String::strncmp(lookupTable + tableIndex, idString, 5)) //It is a match
        {
            char* returnString = new char[strLength + 1];
            MemoryOperations::memcpy(returnString, lookupTable + tableIndex, strLength);
            returnString[strLength] = '\0';

            delete classIDString;
            delete subClassIDString;
            return returnString;
        }

        tableIndex += strLength + 1;
    }

    delete classIDString;
    delete subClassIDString;
    return "Unkown";
}

BaseAddressRegister PCIController::GetBaseAddressRegister(uint16_t bus, uint16_t device, uint16_t function, uint16_t bar)
{
    BaseAddressRegister result;
    MemoryOperations::memset(&result, 0, sizeof(BaseAddressRegister));

    uint32_t barRegister = 0x10 + (bar * sizeof(uint32_t));
    uint32_t barValue = Read(bus, device, function, barRegister);

    if(barValue == 0)
        return result;
    
    uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;
    int maxBARs = 6 - (4*headertype);
    if(bar >= maxBARs)
        return result;

    //Get The size of the BAR
    Write(bus, device, function, barRegister, 0xffffffff);
    uint32_t sizeMask = Read(bus, device, function, barRegister);
    //Write back the original value
    Write(bus, device, function, barRegister, barValue);

    //Fill in the structure
    result.type = (barValue & 0x1) ? InputOutput : MemoryMapping;

    if(result.type == MemoryMapping)
    {
        switch((barValue >> 1) & 0x3)
        {
            case 0: // 32 Bit Mode
                result.address = (uint32_t)(uintptr_t)(barValue & ~0xf);
                result.size = ~(sizeMask & ~0xf) + 1;
                result.prefetchable = barValue & 0x8;
                break;
            case 2: // 64 Bit Mode
                result.size = ~(sizeMask & ~0xf) + 1;
                result.prefetchable = barValue & 0x8;
                BaseAddressRegister sBAR = GetBaseAddressRegister(bus, device, function, bar + 1);
                result.address = ((uint32_t)(uintptr_t)(barValue & ~0xf) + ((sBAR.address & 0xFFFFFFFF) << 32));
                break;
        }   
    }
    else // InputOutput
    {
        result.address = (uint32_t)(barValue & ~0x3);
        result.size = (uint16_t)(~(sizeMask & ~0x3) + 1);
    }
    return result;
}