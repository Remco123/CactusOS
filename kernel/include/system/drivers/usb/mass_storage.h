#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__MASS_STORAGE_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__MASS_STORAGE_H

#include <system/drivers/usb/usbdriver.h>
#include <system/disks/disk.h>
#include <system/tasking/lock.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define SCSI_TEST_UNIT_READY        0x00
            #define SCSI_REQUEST_SENSE          0x03
            #define SCSI_INQUIRY                0x12
            #define SCSI_READ_FORMAT_CAPACITIES 0x23
            #define SCSI_READ_CAPACITY_10       0x25
            #define SCSI_READ_CAPACITY_16       0x9E
            #define SCSI_READ_10                0x28
            #define SCSI_WRITE_10               0x2A
            #define SCSI_READ_16                0x88
            #define SCSI_WRITE_16               0x8A
            //#define SCSI_READ_12                0xA8
            //#define SCSI_WRITE_12               0xAA

            #define CBW_SIGNATURE 0x43425355
            #define CSW_SIGNATURE 0x53425355

            typedef struct 
            {
                uint32_t signature;     // USBC in hexadecimal, acting as magic number
                uint32_t tag;           // Signature
                uint32_t transferLen;   // Number of bytes to transfer excluding size of CBW
                uint8_t flags;          // 7: 0=Out 1=In, 6:0=Reserved
                uint8_t lun;            // 7:4 Reserved, 3:0 Logical Unit Number
                uint8_t cmdLen;         // Length of command in next field [1-16]
                uint8_t command[16];    // Command Data
            } __attribute__((packed)) CommandBlockWrapper;

            typedef struct
            {
                uint32_t signature;     // CSW Magic number
                uint32_t tag;           // Signature, same as CBW
                uint32_t dataResidue;   // Difference in data actually read/written
                uint8_t status;         // Status Byte
            } __attribute__((packed)) CommandStatusWrapper;

            typedef struct
            {
                uint8_t peripheralDeviceType : 5;
                uint8_t peripheralQualifier : 3;
                uint8_t resv1 : 7;
                uint8_t RMB : 1;
                uint8_t version;
                uint8_t responseDataFormat : 4;
                uint8_t HiSup : 1;
                uint8_t NormACA : 1;
                uint8_t resv2 : 2;
                uint8_t additionalLength;
                uint8_t prot : 1;
                uint8_t resv3 : 2;
                uint8_t _3PC : 1;
                uint8_t TPGS : 2;
                uint8_t ACC : 1;
                uint8_t SCCS : 1;
                uint8_t addr16 : 1;
                uint8_t resv4 : 3;
                uint8_t multiP : 1;
                uint8_t VS1 : 1;
                uint8_t ENCServ : 1;
                uint8_t resv5 : 1;
                uint8_t VS2 : 1;
                uint8_t cmndQue : 1;
                uint8_t resv6 : 2;
                uint8_t SYNC : 1;
                uint8_t wbus16 : 1;
                uint8_t resv7 : 2;
                char vendorInformation[8];
                char productIdentification[16];
                char productRevisionLevel[4];
            } __attribute__((packed)) InquiryReturnBlock;

            typedef struct
            {
                uint8_t reserved[3];
                uint8_t listLength;
            } __attribute__((packed)) CapacityListHeader;

            typedef struct
            {
                uint32_t numberOfBlocks;
                uint8_t descriptorCode : 2;
                uint8_t reserved : 6;
                uint32_t blockLength : 24;
            } __attribute__((packed)) CapacityDescriptor;

            typedef struct
            {
                uint32_t logicalBlockAddress;
                uint32_t blockLength;
            } __attribute__((packed)) Capacity10Block;

            typedef struct
            {
                uint64_t logicalBlockAddress;
                uint32_t blockLength;
                uint8_t unused[20];
            } __attribute__((packed)) Capacity16Block;

            typedef struct
            {
                uint8_t errorCode : 7;
                uint8_t valid : 1;
                uint8_t resv1;
                uint8_t senseKey : 4;
                uint8_t resv2 : 1;
                uint8_t ILI : 1;
                uint8_t EOM : 1;
                uint8_t fileMark : 1;
                uint8_t information[4];
                uint8_t additionalLength;
                uint8_t resv3[4];
                uint8_t ASC;
                uint8_t ASCQ;
                uint8_t FRUC;
                uint8_t specific[3];
            } __attribute__((packed)) RequestSenseBlock;

            class USBMassStorageDriver : public USBDriver, public Disk
            {
            private:
                int bulkInEP = 0;   // Endpoint number of bulk in
                int bulkOutEP = 0;  // Endpoint number of bulk out
                int maxLUN = 0;     // Number of Logical Units
                
                bool use16Base = false; // Should we read/write using the 16 command

                MutexLock readWriteLock;
            public:
                // Create new driver for a MSD
                USBMassStorageDriver(USBDevice* dev);

                // Perform a SCSI In or Out operation on device
                bool SCSIRequest(CommandBlockWrapper* request, uint8_t* dataPointer, int dataLength);

                // Prepare Command Block for a specific request
                CommandBlockWrapper SCSIPrepareCommandBlock(uint8_t command, int length, uint64_t lba = 0, int sectors = 0);
                
                // Perform a reset recovery process after stall
                bool ResetRecovery();

                // Called when mass storage device is plugged into system
                bool Initialize() override;

                // Called when mass storage device is unplugged from system
                void DeInitialize() override;

                // Read Sector from mass storage device
                char ReadSector(common::uint32_t lba, common::uint8_t* buf) override;
                // Write Sector to mass storage device
                char WriteSector(common::uint32_t lba, common::uint8_t* buf) override;
            };
        }
    }
}

#endif