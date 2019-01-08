#ifndef __CACTUSOS__SYSTEM__DRIVERS__DISK_CONTROLLERS__IDE_H
#define __CACTUSOS__SYSTEM__DRIVERS__DISK_CONTROLLERS__IDE_H

/*//// Source /////
https://wiki.osdev.org/PCI_IDE_Controller
/////////////////*/

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define ATA_SR_BSY     0x80    // Busy
            #define ATA_SR_DRDY    0x40    // Drive ready
            #define ATA_SR_DF      0x20    // Drive write fault
            #define ATA_SR_DSC     0x10    // Drive seek complete
            #define ATA_SR_DRQ     0x08    // Data request ready
            #define ATA_SR_CORR    0x04    // Corrected data
            #define ATA_SR_IDX     0x02    // Index
            #define ATA_SR_ERR     0x01    // Error

            #define ATA_ER_BBK      0x80    // Bad block
            #define ATA_ER_UNC      0x40    // Uncorrectable data
            #define ATA_ER_MC       0x20    // Media changed
            #define ATA_ER_IDNF     0x10    // ID mark not found
            #define ATA_ER_MCR      0x08    // Media change request
            #define ATA_ER_ABRT     0x04    // Command aborted
            #define ATA_ER_TK0NF    0x02    // Track 0 not found
            #define ATA_ER_AMNF     0x01    // No address mark

            #define ATA_CMD_READ_PIO          0x20
            #define ATA_CMD_READ_PIO_EXT      0x24
            #define ATA_CMD_READ_DMA          0xC8
            #define ATA_CMD_READ_DMA_EXT      0x25
            #define ATA_CMD_WRITE_PIO         0x30
            #define ATA_CMD_WRITE_PIO_EXT     0x34
            #define ATA_CMD_WRITE_DMA         0xCA
            #define ATA_CMD_WRITE_DMA_EXT     0x35
            #define ATA_CMD_CACHE_FLUSH       0xE7
            #define ATA_CMD_CACHE_FLUSH_EXT   0xEA
            #define ATA_CMD_PACKET            0xA0
            #define ATA_CMD_IDENTIFY_PACKET   0xA1
            #define ATA_CMD_IDENTIFY          0xEC

            #define ATAPI_CMD_READ           0xA8
            #define ATAPI_CMD_EJECT          0x1B

            #define ATA_IDENT_DEVICETYPE   0
            #define ATA_IDENT_CYLINDERS    2
            #define ATA_IDENT_HEADS        6
            #define ATA_IDENT_SECTORS      12
            #define ATA_IDENT_SERIAL       20
            #define ATA_IDENT_MODEL        54
            #define ATA_IDENT_CAPABILITIES 98
            #define ATA_IDENT_FIELDVALID   106
            #define ATA_IDENT_MAX_LBA      120
            #define ATA_IDENT_COMMANDSETS  164
            #define ATA_IDENT_MAX_LBA_EXT  200

            #define IDE_ATA        0x00
            #define IDE_ATAPI      0x01
            
            #define ATA_MASTER     0x00
            #define ATA_SLAVE      0x01

            #define ATA_REG_DATA       0x00
            #define ATA_REG_ERROR      0x01
            #define ATA_REG_FEATURES   0x01
            #define ATA_REG_SECCOUNT0  0x02
            #define ATA_REG_LBA0       0x03
            #define ATA_REG_LBA1       0x04
            #define ATA_REG_LBA2       0x05
            #define ATA_REG_HDDEVSEL   0x06
            #define ATA_REG_COMMAND    0x07
            #define ATA_REG_STATUS     0x07
            #define ATA_REG_SECCOUNT1  0x08
            #define ATA_REG_LBA3       0x09
            #define ATA_REG_LBA4       0x0A
            #define ATA_REG_LBA5       0x0B
            #define ATA_REG_CONTROL    0x0C
            #define ATA_REG_ALTSTATUS  0x0C
            #define ATA_REG_DEVADDRESS 0x0D

            // Channels:
            #define ATA_PRIMARY      0x00
            #define ATA_SECONDARY    0x01
            
            // Directions:
            #define ATA_READ      0x00
            #define ATA_WRITE     0x01

            struct IDEChannelRegisters {
                common::uint16_t base;  // I/O Base.
                common::uint16_t ctrl;  // Control Base
                common::uint16_t bmide; // Bus Master IDE
                common::uint8_t  nIEN;  // nIEN (No Interrupt);
            } __attribute__((packed));

            struct IDEDevice {
                common::uint8_t  Reserved;    // 0 (Empty) or 1 (This Drive really exists).
                common::uint8_t  Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
                common::uint8_t  Drive;       // 0 (Master Drive) or 1 (Slave Drive).
                common::uint16_t Type;        // 0: ATA, 1:ATAPI.
                common::uint16_t Signature;   // Drive Signature
                common::uint16_t Capabilities;// Features.
                common::uint32_t CommandSets; // Command Sets Supported.
                common::uint32_t Size;        // Size in Sectors.
                unsigned char    Model[41];   // Model in string.
            } __attribute__((packed));

            class IDEController;

            class IDEInterruptHandler : public InterruptHandler
            {
            private:
                IDEController* controller;
            public:
                IDEInterruptHandler(IDEController* controller, common::uint32_t number);
                common::uint32_t HandleInterrupt(common::uint32_t esp);
            };

            class IDEController : public Driver
            {
            private:
                PCIDevice* pciDevice;

                IDEChannelRegisters channels[2];
                IDEDevice ideDevices[4];

                IDEInterruptHandler* intHandle1;
                IDEInterruptHandler* intHandle2;

                common::uint8_t PrintErrorCode(common::uint32_t drive, common::uint8_t err);
            protected:
            friend class IDEInterruptHandler;
                //IRQ Handler
                void HandleIRQ();
                void WaitForIRQ();

                common::uint8_t ReadRegister(common::uint8_t channel, common::uint8_t reg);
                void WriteRegister(common::uint8_t channel, common::uint8_t reg, common::uint8_t data);
                void ReadDeviceBuffer(common::uint8_t channel, common::uint8_t reg, common::uint32_t buffer, common::uint32_t quads);
                common::uint8_t Polling(common::uint8_t channel, common::uint32_t advanced_check);

                //Drive specific read/write
                common::uint8_t AtaReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
                common::uint8_t AtaWriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);

                common::uint8_t ATAPIReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            public:
                IDEController(PCIDevice* device);

                bool Initialize();

                //Read Functions
                common::uint8_t ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
                common::uint8_t WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);

                common::uint8_t EjectDrive(common::uint8_t drive);
            };
        }
    }
}

#endif