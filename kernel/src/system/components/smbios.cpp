#include <system/components/smbios.h>

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

    } 
    else if(header->type == SMBIOSTableType::SystemSlotsInformation)
    {

    } 
    else if(header->type == SMBIOSTableType::PhysicalMemoryArray)
    {

    } 
    else if(header->type == SMBIOSTableType::MemoryDevice)
    {

    } 
    else if(header->type == SMBIOSTableType::MemoryArrayMappedAddress)
    {

    } 
    else if(header->type == SMBIOSTableType::MemoryDeviceMappedAddress)
    {

    } 
    else if(header->type == SMBIOSTableType::SystemBootInformation)
    {

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