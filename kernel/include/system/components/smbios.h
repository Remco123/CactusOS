#ifndef __CACTUSOS__SYSTEM__SMBIOS_H
#define __CACTUSOS__SYSTEM__SMBIOS_H

#include <system/components/systemcomponent.h>
#include <system/bootconsole.h>
#include <common/string.h>
#include <common/list.h>

namespace CactusOS
{
    namespace system
    {
        enum SMBIOSTableType 
        {
            BIOSInformation = 0,
            SystemInformation = 1,
            BaseBoardInformation = 2,
            EnclosureInformation = 3,
            ProcessorInformation = 4,
            MemoryControllerInformation = 5,
            MemoryModuleInformation = 6,
            CacheInformation = 7,
            PortConnectorInformation = 8,
            SystemSlotsInformation = 9,
            OnBoardDevicesInformation = 10,
            OEMStrings = 11,
            SystemConfigurationOptions = 12,
            BIOSLanguageInformation = 13,
            GroupAssociations = 14,
            SystemEventLog = 15,
            PhysicalMemoryArray = 16,
            MemoryDevice = 17,
            MemoryErrorInformation = 18,
            MemoryArrayMappedAddress = 19,
            MemoryDeviceMappedAddress = 20,
            SystemBootInformation = 32,
            EndOfTable = 127
        };

        struct SMBIOSTag
        {
            common::uint8_t type;
            common::uint8_t length;
            common::uint16_t handle;
        } __attribute__((packed));

        struct SMBIOSBiosInfo : public SMBIOSTag
        {
            common::uint8_t vendor;
            common::uint8_t version;
            common::uint16_t startSegment;
            common::uint8_t releaseDate;
            common::uint8_t romSize;
            common::uint64_t characteristics;
            // for 2.4+
            common::uint8_t extensionChar1;
            common::uint8_t extensionChar2;
            common::uint8_t biosMajorRelease;
            common::uint8_t biosMinorRelease;
            common::uint8_t controllerMajorRelease;
            common::uint8_t controllerMinorRelease;
        } __attribute__((packed));

        struct SMBIOSSystemInfo : public SMBIOSTag
        {
            common::uint8_t manufacturer;
            common::uint8_t productName;
            common::uint8_t version;
            common::uint8_t serialNumber;

            struct
            {
                common::uint32_t time_low;
                common::uint16_t time_mid;
                common::uint16_t timeHiAndVersion;
                common::uint8_t clockSeqHiAndReserved;
                common::uint8_t clockSeqLow;
                common::uint8_t node[6];
            } uuid;

            common::uint8_t wakeupType;

            common::uint8_t sku;
            common::uint8_t family;
        } __attribute__((packed));

        struct SMBIOSBaseBoardInformation : public SMBIOSTag
        {
            common::uint8_t manufacturer;
            common::uint8_t product;
            common::uint8_t version;
            common::uint8_t serial;
            common::uint8_t assertTag;
            common::uint8_t features;
            common::uint8_t locationInChassis;
            common::uint16_t chassisHandle;
            common::uint8_t boardType;
            common::uint8_t numObjHandles;
            common::uint16_t objectHandles[1]; 
        } __attribute__((packed));

        struct SMBIOSSystemEnclosureInformation : public SMBIOSTag
        {
            common::uint8_t manufacturer;
            common::uint8_t type;
            common::uint8_t version;
            common::uint8_t serialNumber;
            common::uint8_t assetTag;
            common::uint8_t bootupState;
            common::uint8_t psuState;
            common::uint8_t thermalState;
            common::uint8_t securityStatus;
            common::uint8_t oemSpecific[4];
            common::uint8_t height;
            common::uint8_t numOfPowerCords;
            common::uint8_t numOfElements;
            common::uint8_t elementRecordLength;

            struct element
            {
                common::uint8_t type;
                common::uint8_t minimum;
                common::uint8_t maximum;
            } elements[1];
        } __attribute__((packed));

        struct SMBIOSProcessorInformation : public SMBIOSTag
        {
            common::uint8_t socketDesignation;
            common::uint8_t processorType;
            common::uint8_t processorFamily;
            common::uint8_t manufacturer;
            common::uint64_t processorID;
            common::uint8_t version;
            common::uint8_t voltage;
            common::uint16_t clock;
            common::uint16_t maxSpeed;
            common::uint16_t currentSpeed;
            common::uint8_t status;
            common::uint8_t upgrade;
            common::uint16_t L1;
            common::uint16_t L2;
            common::uint16_t L3;

            //2.3
            common::uint8_t serialNumber;
            common::uint8_t assertTag;
            common::uint8_t partNumber;

            //2.5
            common::uint8_t totalCores;
            common::uint8_t activeCores;
            common::uint8_t threads;
            common::uint16_t characteristics;
        } __attribute__((packed));

        struct SMBIOSCacheInformation : public SMBIOSTag
        {
            common::uint8_t socketDesignation;
            common::uint16_t cacheConfiguration;
            common::uint16_t maximumCacheSize;
            common::uint16_t installedSize;
            common::uint16_t supportedSRAMType;
            common::uint16_t currentSRAMType;

            //2.1+
            common::uint8_t cacheSpeed;
            common::uint8_t errorCorrectionType;
            common::uint8_t systemCacheType;
            common::uint8_t associativity;

            //3.1+
            common::uint32_t maximumCacheSize2;
            common::uint32_t installedSize2;
        } __attribute__((packed));

        struct SMBIOSSystemSlotInformation : public SMBIOSTag
        {
            common::uint8_t slotDesignation;
            common::uint8_t slotType;
            common::uint8_t slotDataBusWidth;
            common::uint8_t currentUsage;
            common::uint8_t slotLength;
            common::uint16_t slotID;
            common::uint8_t slotCharacteristics1;

            //2.1+
            common::uint8_t slotCharacteristics2;

            //2.6+
            common::uint16_t segmentGroupNumber;
            common::uint8_t busNumber;
            common::uint8_t deviceFunctionNumber;

            //3.2
            common::uint8_t dataBusWidth;
        } __attribute__((packed));

        struct SMBIOSPhysicalMemoryArray : public SMBIOSTag
        {
            //2.1+
            common::uint8_t location;
            common::uint8_t use;
            common::uint8_t memoryErrorCorrection;
            common::uint32_t maximumCapacity;
            common::uint16_t memoryErrorInformationHandle;
            common::uint16_t numberOfMemoryDevices;

            //2.7+
            common::uint64_t extendedMaximumCapacity;
        } __attribute__((packed));

        struct SMBIOSMemoryDevice : public SMBIOSTag
        {
            //2.1+
            common::uint16_t memoryArrayHandle;
            common::uint16_t memoryErrorInformationHandle;
            common::uint16_t totalWidth;
            common::uint16_t dataWidth;
            common::uint16_t size;
            common::uint8_t formFactor;
            common::uint8_t deviceSet;
            common::uint8_t deviceLocator;
            common::uint8_t bankLocator;
            common::uint8_t memoryType;
            common::uint16_t typeDetail;

            //2.3+
            common::uint16_t speed;
            common::uint8_t manufacturer;
            common::uint8_t serialNumber;
            common::uint8_t assetTag;
            common::uint8_t partNumber;

            //2.6+
            common::uint8_t attributes;

            //2.7+
            common::uint32_t extendedSize;
            common::uint16_t configuredMemorySpeed;

            //2.8+
            common::uint16_t minimumVoltage;
            common::uint16_t maximumVoltage;
            common::uint16_t configuredVoltage;

            //3.2+
            common::uint8_t memoryTechnology;
            common::uint16_t memoryOperatingModeCapability;
            common::uint8_t firwareVersion;
            common::uint16_t moduleManufacturerID;
            common::uint16_t moduleProductID;
            common::uint16_t memorySubsystemControllerManufacturerID;
            common::uint16_t memorySubsystemControllerProductID;
            common::uint64_t nonVolatileSize;
            common::uint64_t volatileSize;
            common::uint64_t cacheSize;
            common::uint64_t logicalSize;
        } __attribute__((packed));

        struct SMBIOSBootInformation : public SMBIOSTag
        {
            common::uint8_t reserved[6];
            common::uint8_t bootStatus;
        } __attribute__((packed));

        struct SMBIOSEntryPoint
        {
            char EntryPointString[4];               //This is _SM_
            common::uint8_t Checksum;               //This value summed with all the values of the table, should be 0 (overflow)
            common::uint8_t Length;                 //Length of the Entry Point Table. Since version 2.1 of SMBIOS, this is 0x1F
            common::uint8_t MajorVersion;           //Major Version of SMBIOS
            common::uint8_t MinorVersion;           //Minor Version of SMBIOS
            common::uint16_t MaxStructureSize;      //Maximum size of a SMBIOS Structure (we will se later)
            common::uint8_t EntryPointRevision;     //...
            char FormattedArea[5];                  //...
            char EntryPointString2[5];              //This is _DMI_
            common::uint8_t Checksum2;              //Checksum for values from EntryPointString2 to the end of table
            common::uint16_t TableLength;           //Length of the Table containing all the structures
            common::uint32_t TableAddress;	        //Address of the Table
            common::uint16_t NumberOfStructures;    //Number of structures in the table
            common::uint8_t BCDRevision;            //Unused
        } __attribute__((packed));

        class SMBIOS : public SystemComponent
        {
        private:
            void* TableAddress = 0;

            List<char*> ExtractStrings(SMBIOSTag* header);

            char* CopyString(char* src);
        public:
            SMBIOS();
        };
    }
}

#endif