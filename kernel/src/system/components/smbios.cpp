#include <system/components/smbios.h>
#include <system/system.h>
#include <system/listings/systeminfo.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

char* SMBIOS::CopyString(char* src)
{
    int len = String::strlen(src);
    char* ret = new char[len + 1];
    MemoryOperations::memcpy(ret, src, len);
    ret[len] = '\0';
    return ret;
}

SMBIOS::SMBIOS()
: SystemComponent("SMBIOS", "System Management BIOS")
{
    Log(Info, "Searching for SMBIOS structure");

    char* memAddress = (char*)0xF0000;
    while ((uint32_t)memAddress < 0x100000) {
        if (memAddress[0] == '_' && memAddress[1] == 'S' && memAddress[2] == 'M' && memAddress[3] == '_') {
            int length = memAddress[5];
            uint8_t checksum = 0;
            for(int i = 0; i < length; i++) {
                checksum += memAddress[i];
            }
            if(checksum == 0) break;
        }
        memAddress += 16;
    }

    if((uint32_t)memAddress != 0x100000)
    {
        Log(Info, "Found at: %x", (uint32_t)memAddress);
        this->TableAddress = memAddress;

        SMBIOSEntryPoint* entryPoint = (SMBIOSEntryPoint*)this->TableAddress;
        Log(Info, " -------- SMBIOS Summary ----------");
        Log(Info, "Version: %d.%d", entryPoint->MajorVersion, entryPoint->MinorVersion);
        Log(Info, "Number of Structures: %d", entryPoint->NumberOfStructures);
        Log(Info, "Table Address: %x", entryPoint->TableAddress);
        Log(Info, "Table Length: %d", entryPoint->TableLength);
        Log(Info, " ---------------------------------");

        int i = 0;
        uint32_t tableAddress = entryPoint->TableAddress;
        while(i < entryPoint->NumberOfStructures)
        {
            SMBIOSTag* tag = (SMBIOSTag*)tableAddress;
            List<char*> stringList = ExtractStrings(tag);
            switch(tag->type) {
                case SMBIOSTableType::BIOSInformation:
                {
                    SMBIOSBiosInfo* info = (SMBIOSBiosInfo*)tag;
                    SystemInfoManager::bios.vendor = CopyString(stringList[info->vendor]);
                    SystemInfoManager::bios.version = CopyString(stringList[info->version]);
                    SystemInfoManager::bios.releaseDate = CopyString(stringList[info->releaseDate]);

                    #if BOCHS_GFX_HACK // Massive hack to detect bochs so that we can use the right video device, TODO: Improve
                    System::isBochs = String::strcmp(stringList[info->vendor], "The Bochs Project");
                    if(System::isBochs)
                        BootConsole::WriteLine("(Warning) Using Bochs GFX Hack");
                    #endif
                    break;
                }
                case SMBIOSTableType::SystemInformation:
                {
                    SMBIOSSystemInfo* info = (SMBIOSSystemInfo*)tag;
                    SystemInfoManager::system.manufacturer = CopyString(stringList[info->manufacturer]);
                    SystemInfoManager::system.product = CopyString(stringList[info->productName]);
                    SystemInfoManager::system.version = CopyString(stringList[info->version]);
                    SystemInfoManager::system.serial = CopyString(stringList[info->serialNumber]);
                    
                    if(entryPoint->MajorVersion >= 2 && entryPoint->MinorVersion > 4) {
                        SystemInfoManager::system.sku = CopyString(stringList[info->sku]);
                        SystemInfoManager::system.family = CopyString(stringList[info->family]);
                    }
                    else {
                        SystemInfoManager::system.sku = "N/A";
                        SystemInfoManager::system.family = "N/A";
                    }
                    break;
                }
                case SMBIOSTableType::EnclosureInformation:
                {
                    SMBIOSSystemEnclosureInformation* info = (SMBIOSSystemEnclosureInformation*)tag;
                    SystemInfoManager::enclosure.manufacturer = CopyString(stringList[info->manufacturer]); 
                    SystemInfoManager::enclosure.serial = CopyString(stringList[info->serialNumber]); 
                    SystemInfoManager::enclosure.sku = CopyString(stringList[info->assetTag]); 
                    SystemInfoManager::enclosure.version = CopyString(stringList[info->version]);                     
                    break;
                }
                case SMBIOSTableType::ProcessorInformation:
                {
                    SMBIOSProcessorInformation* info = (SMBIOSProcessorInformation*)tag;
                    SystemInfoManager::processor.manufacturer = CopyString(stringList[info->manufacturer]);
                    SystemInfoManager::processor.socket = CopyString(stringList[info->socketDesignation]);
                    SystemInfoManager::processor.version = CopyString(stringList[info->version]);
                    break;
                }
                case SMBIOSTableType::EndOfTable:
                    return;
            }

            tableAddress += ((SMBIOSTag*)tableAddress)->length;

            while(0 != (*((uint8_t*)tableAddress) | *((uint8_t*)tableAddress + 1))) tableAddress++;

            tableAddress += 2;
            i++;
        }
    }
}

List<char*> SMBIOS::ExtractStrings(SMBIOSTag* header)
{
    List<char*> stringList;
    stringList.push_back("N/A");

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
            stringList.push_back(str);
    }

    return stringList;
}