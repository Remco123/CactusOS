#include <system/components/smbios.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

uint8_t BiosMajorVersion = 0;
uint8_t BiosMinorVersion = 0;

SMBIOS::SMBIOS(bool searchForTable)
: SystemComponent("SMBIOS", "System Management BIOS")
{
    if(searchForTable)
    {
        BootConsole::WriteLine("Searching for SMBIOS structure");

        char* memAddress = (char*)0xF0000;
        int length, i;
        unsigned char checksum;
        while ((uint32_t) memAddress < 0x100000) {
            if (memAddress[0] == '_' && memAddress[1] == 'S' && memAddress[2] == 'M' && memAddress[3] == '_') {
                length = memAddress[5];
                checksum = 0;
                for(i = 0; i < length; i++) {
                    checksum += memAddress[i];
                }
                if(checksum == 0) break;
            }
            memAddress += 16;
        }

        if((uint32_t)memAddress != 0x100000)
        {
            BootConsole::Write("Found at: 0x"); Print::printfHex32((uint32_t)memAddress); BootConsole::WriteLine();
            this->TableAddress = memAddress;

#if BOCHS_GFX_HACK //Massive hack to detect bochs so that we can use the right video device, TODO: Improve
            SMBIOSEntryPoint* entryPoint = (SMBIOSEntryPoint*)this->TableAddress;
            int i = 0;
            uint32_t tableAddress = entryPoint->TableAddress;
            while(i < entryPoint->NumberOfStructures)
            {
                i++;

                SMBIOSTag* tag = (SMBIOSTag*)tableAddress;
                if(tag->type == SMBIOSTableType::BIOSInformation)
                {
                    List<char*>* stringList = ExtractStrings(tag);
                    SMBIOSBiosInfo* info = (SMBIOSBiosInfo*)tag;
                    System::isBochs = String::strcmp(stringList->GetAt(info->vendor), "The Bochs Project");
                    if(System::isBochs)
                        BootConsole::WriteLine("(Warning) Using Bochs GFX Hack");
                    delete stringList;
                }

                tableAddress += ((SMBIOSTag*)tableAddress)->length;

                while(0 != (*((uint8_t*)tableAddress) | *((uint8_t*)tableAddress + 1))) tableAddress++;

                tableAddress += 2;
            }
#endif
        }
    }
}

void SMBIOS::PrintHeaderSummary(SMBIOSTag* header)
{
    if(header->type == SMBIOSTableType::EndOfTable)
        return;
    
    //BootConsole::WriteLine("#---- SMBIOS Header ----#");
    //BootConsole::Write("Header type: "); BootConsole::WriteLine(Convert::IntToString(header->type));
    //BootConsole::Write("Header length: "); BootConsole::WriteLine(Convert::IntToString(header->length));

    List<char*>* stringList = ExtractStrings(header);
    //BootConsole::Write("Header strings: "); BootConsole::WriteLine(Convert::IntToString(stringList->size()));


    if(header->type == SMBIOSTableType::BIOSInformation)
    {
        BootConsole::WriteLine("- BIOS Information Header -");
        SMBIOSBiosInfo* info = (SMBIOSBiosInfo*)header;

        BootConsole::Write("Vendor: "); BootConsole::WriteLine(stringList->GetAt(info->vendor));
        BootConsole::Write("Version: "); BootConsole::WriteLine(stringList->GetAt(info->version));
        BootConsole::Write("Release Date: "); BootConsole::WriteLine(stringList->GetAt(info->releaseDate));

        BootConsole::WriteLine("---------------------------");
    }
    else if(header->type == SMBIOSTableType::SystemInformation)
    {
        BootConsole::WriteLine("- System Information Header -");
        SMBIOSSystemInfo* info = (SMBIOSSystemInfo*)header;

        BootConsole::Write("Manufacturer: "); BootConsole::WriteLine(stringList->GetAt(info->manufacturer));
        BootConsole::Write("Product: "); BootConsole::WriteLine(stringList->GetAt(info->productName));
        BootConsole::Write("Version: "); BootConsole::WriteLine(stringList->GetAt(info->version));
        BootConsole::Write("Serial: "); BootConsole::WriteLine(stringList->GetAt(info->serialNumber));

        BootConsole::Write("Wakeup Type: "); BootConsole::WriteLine(Convert::IntToString(info->wakeupType));

        //Only supported for 2.4+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 4) {
            BootConsole::Write("SKU: "); BootConsole::WriteLine(stringList->GetAt(info->sku));
            BootConsole::Write("Family: "); BootConsole::WriteLine(stringList->GetAt(info->family));
        }

        BootConsole::WriteLine("-----------------------------");
    }   
    else if(header->type == SMBIOSTableType::BaseBoardInformation)
    {
        BootConsole::WriteLine("- Baseboard Information Header -");
        SMBIOSBaseBoardInformation* info = (SMBIOSBaseBoardInformation*)header;

        BootConsole::Write("Manufacturer: "); BootConsole::WriteLine(stringList->GetAt(info->manufacturer));
        BootConsole::Write("Product: "); BootConsole::WriteLine(stringList->GetAt(info->product));
        BootConsole::Write("Version: "); BootConsole::WriteLine(stringList->GetAt(info->version));
        BootConsole::Write("Serial: "); BootConsole::WriteLine(stringList->GetAt(info->serial));
        BootConsole::Write("Board Type: "); BootConsole::WriteLine(Convert::IntToString(info->boardType));

        BootConsole::WriteLine("--------------------------------");
    } 
    else if(header->type == SMBIOSTableType::EnclosureInformation)
    {
        BootConsole::WriteLine("- Enclosure Information Header -");
        SMBIOSSystemEnclosureInformation* info = (SMBIOSSystemEnclosureInformation*)header;

        BootConsole::Write("Manufacturer: "); BootConsole::WriteLine(stringList->GetAt(info->manufacturer));
        BootConsole::Write("Type: "); BootConsole::WriteLine(Convert::IntToString(info->type));
        BootConsole::Write("Version: "); BootConsole::WriteLine(stringList->GetAt(info->version));
        BootConsole::Write("Serial: "); BootConsole::WriteLine(stringList->GetAt(info->serialNumber));
        BootConsole::Write("Asset Tag: "); BootConsole::WriteLine(stringList->GetAt(info->assetTag));
        
        BootConsole::Write("Bootup State: "); BootConsole::WriteLine(Convert::IntToString(info->bootupState));
        BootConsole::Write("PSU State: "); BootConsole::WriteLine(Convert::IntToString(info->psuState));
        BootConsole::Write("Thermal State: "); BootConsole::WriteLine(Convert::IntToString(info->thermalState));
        BootConsole::Write("Security Status: "); BootConsole::WriteLine(Convert::IntToString(info->securityStatus));

        //2.3+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 3) {
            BootConsole::Write("Height (cm): "); BootConsole::WriteLine(Convert::IntToString(info->height * 4.445));
            BootConsole::Write("Power Records: "); BootConsole::WriteLine(Convert::IntToString(info->numOfPowerCords));
        }

        BootConsole::WriteLine("--------------------------------");
    } 
    else if(header->type == SMBIOSTableType::ProcessorInformation)
    {
        BootConsole::WriteLine("- Processor Information Header -");
        SMBIOSProcessorInformation* info = (SMBIOSProcessorInformation*)header;

        BootConsole::Write("Socket Designation: "); BootConsole::WriteLine(stringList->GetAt(info->socketDesignation));
        BootConsole::Write("Processor Type: "); BootConsole::WriteLine(Convert::IntToString(info->processorType));
        BootConsole::Write("Processor Family: "); BootConsole::WriteLine(Convert::IntToString(info->processorFamily));
        BootConsole::Write("Manufacturer: "); BootConsole::WriteLine(stringList->GetAt(info->manufacturer));
        BootConsole::Write("Version: "); BootConsole::WriteLine(stringList->GetAt(info->version));

        BootConsole::Write("Voltage: ");
        if ((info->voltage >> 7) & 1)
        {
            BootConsole::Write(Convert::IntToString((info->voltage & 0b1111111)/10.0)); BootConsole::WriteLine("V");
        }
        else
        {
            BootConsole::Write((char*)(info->voltage == 0 ? "5" : info->voltage == 1 ? "3.3" : info->voltage == 2 ? "2.9" : "-1")); BootConsole::WriteLine("V");
        }

        BootConsole::Write("Clock Speed: "); BootConsole::Write(Convert::IntToString(info->clock)); BootConsole::WriteLine(" MHz");
        BootConsole::Write("Max Speed: "); BootConsole::Write(Convert::IntToString(info->maxSpeed)); BootConsole::WriteLine(" MHz");
        BootConsole::Write("Current Speed: "); BootConsole::Write(Convert::IntToString(info->currentSpeed)); BootConsole::WriteLine(" MHz");
        BootConsole::Write("Upgrade: "); BootConsole::WriteLine(Convert::IntToString(info->upgrade));

        //2.5+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 5) {
            BootConsole::Write("Cores: "); BootConsole::WriteLine(Convert::IntToString(info->totalCores));
            BootConsole::Write("Enabled Cores: "); BootConsole::WriteLine(Convert::IntToString(info->activeCores));
            BootConsole::Write("Threads: "); BootConsole::WriteLine(Convert::IntToString(info->threads));
        }

        BootConsole::WriteLine("--------------------------------");
    } 
    else if(header->type == SMBIOSTableType::CacheInformation)
    {
        BootConsole::WriteLine("- Cache Information Header -");
        SMBIOSCacheInformation* info = (SMBIOSCacheInformation*)header;

        BootConsole::Write("Socket Designation: "); BootConsole::WriteLine(stringList->GetAt(info->socketDesignation));
        
        //2.1+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 1) {
            BootConsole::Write("Cache Speed: "); BootConsole::Write(Convert::IntToString(info->cacheSpeed)); BootConsole::WriteLine(" Ns");
        }

        BootConsole::WriteLine("----------------------------");
    } 
    else if(header->type == SMBIOSTableType::SystemSlotsInformation)
    {
        BootConsole::WriteLine("- System Slots Information Header -");
        SMBIOSSystemSlotInformation* info = (SMBIOSSystemSlotInformation*)header;

        BootConsole::Write("Slot Designation: "); BootConsole::WriteLine(stringList->GetAt(info->slotDesignation));
        BootConsole::Write("Slot Type: "); BootConsole::WriteLine(Convert::IntToString(info->slotType));
        BootConsole::Write("Slot Data Bus Width: "); BootConsole::WriteLine(Convert::IntToString(info->slotDataBusWidth));
        BootConsole::Write("Current Usage: "); BootConsole::WriteLine(Convert::IntToString(info->currentUsage));
        BootConsole::Write("Slot Length: "); BootConsole::WriteLine(Convert::IntToString(info->slotLength));

        BootConsole::WriteLine("-----------------------------------");
    } 
    else if(header->type == SMBIOSTableType::PhysicalMemoryArray)
    {
        BootConsole::WriteLine("- Physical Memory Array Header -");
        SMBIOSPhysicalMemoryArray* info = (SMBIOSPhysicalMemoryArray*)header;

        //2.1+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 1)
        {
            BootConsole::Write("Location: "); BootConsole::WriteLine(Convert::IntToString(info->location));
            BootConsole::Write("Use: "); BootConsole::WriteLine(Convert::IntToString(info->use));
            BootConsole::Write("Memory Error Correction: "); BootConsole::WriteLine(Convert::IntToString(info->memoryErrorCorrection));
            BootConsole::Write("Maximum Capacity: "); BootConsole::Write(Convert::IntToString(info->maximumCapacity)); BootConsole::WriteLine(" Kb");
            BootConsole::Write("Number of Mem-Devices: "); BootConsole::WriteLine(Convert::IntToString(info->numberOfMemoryDevices));
        }

        BootConsole::WriteLine("--------------------------------");
    } 
    else if(header->type == SMBIOSTableType::MemoryDevice)
    {
        BootConsole::WriteLine("- Memory Device Header -");
        SMBIOSMemoryDevice* info = (SMBIOSMemoryDevice*)header;

        //2.1+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 1)
        {
            BootConsole::Write("Total Width: "); BootConsole::WriteLine(Convert::IntToString(info->totalWidth));
            BootConsole::Write("Data Width: "); BootConsole::WriteLine(Convert::IntToString(info->dataWidth));
            BootConsole::Write("Size: "); BootConsole::WriteLine(Convert::IntToString(info->size));
            BootConsole::Write("Form Factor: "); BootConsole::WriteLine(Convert::IntToString(info->formFactor));
            BootConsole::Write("Device Set: "); BootConsole::WriteLine(Convert::IntToString(info->deviceSet));
            BootConsole::Write("Device Locator: "); BootConsole::WriteLine(stringList->GetAt(info->deviceLocator));
            BootConsole::Write("Bank Locator: "); BootConsole::WriteLine(stringList->GetAt(info->bankLocator));
            BootConsole::Write("Memory Type: "); BootConsole::WriteLine(Convert::IntToString(info->memoryType));
        }

        //2.3+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 3)
        {
            BootConsole::Write("Speed: "); BootConsole::Write(Convert::IntToString(info->speed)); BootConsole::WriteLine(" MT/s");
            BootConsole::Write("Manufacturer: "); BootConsole::WriteLine(stringList->GetAt(info->manufacturer));
            BootConsole::Write("Serial Number: "); BootConsole::WriteLine(stringList->GetAt(info->serialNumber));
            BootConsole::Write("Asset Tag: "); BootConsole::WriteLine(stringList->GetAt(info->assetTag));
            BootConsole::Write("Part Number: "); BootConsole::WriteLine(stringList->GetAt(info->partNumber));
        }

        //2.8+
        if(BiosMajorVersion >= 2 && BiosMinorVersion > 8)
        {
            BootConsole::Write("Minimum Voltage: "); BootConsole::Write(Convert::IntToString(info->minimumVoltage)); BootConsole::WriteLine(" MiV");
            BootConsole::Write("Maximum Voltage: "); BootConsole::Write(Convert::IntToString(info->maximumVoltage)); BootConsole::WriteLine(" MiV");
            BootConsole::Write("Configured Voltage: "); BootConsole::Write(Convert::IntToString(info->configuredVoltage)); BootConsole::WriteLine(" MiV");
        }

        BootConsole::WriteLine("------------------------");
    }
    else if(header->type == SMBIOSTableType::SystemBootInformation)
    {
        BootConsole::WriteLine("- System Boot Information Header -");
        SMBIOSBootInformation* info = (SMBIOSBootInformation*)header;

        //2.3
        if(BiosMajorVersion >= 2 && BiosMinorVersion >= 3)
        {
            BootConsole::Write("Boot Status: "); BootConsole::WriteLine(Convert::IntToString(info->bootStatus));
        }

        BootConsole::WriteLine("----------------------------------");
    } 
    else  //Unkown header type
    {
        BootConsole::Write("- Unkown Header type: "); BootConsole::Write(Convert::IntToString(header->type)); BootConsole::WriteLine(" -");
    }

    delete stringList;

    //BootConsole::WriteLine("#-----------------------#");
}

List<char*>* SMBIOS::ExtractStrings(SMBIOSTag* header)
{
    List<char*>* stringList = new List<char*>();
    stringList->Clear();
    stringList->push_back("N/A");

    uint8_t* ptr = ((uint8_t*)header) + header->length;

    if(*ptr == 0) 
        ptr += 2;
    else for (;;) {
        char* str = (char*)ptr;
        uint32_t len = String::strlen(str);

        ptr += len + 1;

        if(len == 0)
            break;
        else
            stringList->push_back(str);
    }

    return stringList;
}

void SMBIOS::PrintSummary()
{
    if(this->TableAddress != 0)
    {
        BootConsole::WriteLine("SMBIOS Summary");
        SMBIOSEntryPoint* entryPoint = (SMBIOSEntryPoint*)this->TableAddress;
        BootConsole::Write("Version: "); BootConsole::Write(Convert::IntToString(entryPoint->MajorVersion)); BootConsole::Write("."); BootConsole::WriteLine(Convert::IntToString(entryPoint->MinorVersion));
        BootConsole::Write("Number of Structures: "); BootConsole::WriteLine(Convert::IntToString(entryPoint->NumberOfStructures));
        BootConsole::Write("Table Address: 0x"); Print::printfHex32(entryPoint->TableAddress); BootConsole::WriteLine();
        BootConsole::Write("Table Length: "); BootConsole::WriteLine(Convert::IntToString(entryPoint->TableLength));

        BiosMajorVersion = entryPoint->MajorVersion;
        BiosMinorVersion = entryPoint->MinorVersion;

        int i = 0;
        uint32_t tableAddress = entryPoint->TableAddress;
        while(i < entryPoint->NumberOfStructures)
        {
            i++;

            PrintHeaderSummary((SMBIOSTag*)tableAddress);

            tableAddress += ((SMBIOSTag*)tableAddress)->length;

            while(0 != (*((uint8_t*)tableAddress) | *((uint8_t*)tableAddress + 1))) tableAddress++;

            tableAddress += 2;
        }
    }
}