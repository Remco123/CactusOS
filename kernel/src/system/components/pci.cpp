#include <system/components/pci.h>

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
    BootConsole::WriteLine("Scanning for PCI Devices");

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

                BootConsole::Write(Convert::IntToString(bus)); BootConsole::Write(":"); 
                BootConsole::Write(Convert::IntToString(device)); BootConsole::Write(":");
                BootConsole::Write(Convert::IntToString(function));

                //Add pci device to list
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

                //Get the portBase from the base address registers
                for(int barNum = 0; barNum < 6; barNum++)
                {
                    BaseAddressRegister bar = GetBaseAddressRegister(bus, device, function, barNum);
                    if(bar.address && (bar.type == InputOutput))
                        pciDevice->portBase = (uint32_t)bar.address;
                }

                BootConsole::SetX(8); BootConsole::Write("  ");
                Print::printfHex16(pciDevice->vendorID); BootConsole::Write(":");
                Print::printfHex16(pciDevice->deviceID); BootConsole::Write("  ");
                BootConsole::WriteLine(GetClassCodeString(pciDevice->classID, pciDevice->subclassID));
                
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