#ifndef __CACTUSOS__SYSTEM__DRIVERS__DISK_CONTROLLERS__IDE_H
#define __CACTUSOS__SYSTEM__DRIVERS__DISK_CONTROLLERS__IDE_H

/*//// Source /////
https://wiki.osdev.org/PCI_IDE_Controller
/////////////////*/

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>

#include <system/disks/diskcontroller.h>
#include <system/tasking/lock.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define IDE_SR_BSY     0x80    // Busy
            #define IDE_SR_DRDY    0x40    // Drive ready
            #define IDE_SR_DF      0x20    // Drive write fault
            #define IDE_SR_DSC     0x10    // Drive seek complete
            #define IDE_SR_DRQ     0x08    // Data request ready
            #define IDE_SR_CORR    0x04    // Corrected data
            #define IDE_SR_IDX     0x02    // Index
            #define IDE_SR_ERR     0x01    // Error

            #define IDE_ER_BBK      0x80    // Bad block
            #define IDE_ER_UNC      0x40    // Uncorrectable data
            #define IDE_ER_MC       0x20    // Media changed
            #define IDE_ER_IDNF     0x10    // ID mark not found
            #define IDE_ER_MCR      0x08    // Media change request
            #define IDE_ER_ABRT     0x04    // Command aborted
            #define IDE_ER_TK0NF    0x02    // Track 0 not found
            #define IDE_ER_AMNF     0x01    // No address mark

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

            #define ATA_IDENT_MODEL        54
            #define ATA_IDENT_MAX_LBA      60
            #define ATA_IDENT_COMMANDSETS  83
            #define ATA_IDENT_MAX_LBA_EXT  100

            #define IDE_ATA        0x00
            #define IDE_ATAPI      0x01
            
            #define ATA_MASTER     0x00
            #define ATA_SLAVE      0x01

            #define IDE_REG_DATA       0x00
            #define IDE_REG_ERROR      0x01
            #define IDE_REG_FEATURES   0x01
            #define IDE_REG_SECCOUNT0  0x02
            #define IDE_REG_LBA0       0x03
            #define IDE_REG_LBA1       0x04
            #define IDE_REG_LBA2       0x05
            #define IDE_REG_HDDEVSEL   0x06
            #define IDE_REG_COMMAND    0x07
            #define IDE_REG_STATUS     0x07
            #define IDE_REG_SECCOUNT1  0x08
            #define IDE_REG_LBA3       0x09
            #define IDE_REG_LBA4       0x0A
            #define IDE_REG_LBA5       0x0B
            #define IDE_REG_CONTROL    0x0C
            #define IDE_REG_ALTSTATUS  0x0C
            #define IDE_REG_DEVADDRESS 0x0D

            #define IDE_CHANNEL_PRIMARY     0x00
            #define IDE_CHANNEL_SECONDARY   0x01

            // Forward declare the used IDEController class
            class IDEController;
            class IDEInterruptHandler : InterruptHandler
            {
            private:
                // To which IDEController is this interrupt handler bound
                IDEController* target = 0;
            public:
                // Create new IDE Interrupt handler
                IDEInterruptHandler(IDEController* parent, uint8_t interrupt);

                // Function called by interrupt
                uint32_t HandleInterrupt(uint32_t esp) override;
            };

            // Holds information about each IDE-Channel
            struct IDEChannel {
                uint16_t commandReg;  // I/O Base.
                uint16_t controlReg;  // Control Base
            };

            // Holds information about each IDE-Device
            struct IDEDevice {
                uint8_t  Channel;      // 0 (Primary Channel) or 1 (Secondary Channel).
                uint8_t  Drive;        // 0 (Master Drive) or 1 (Slave Drive).
                uint16_t Type;         // 0: ATA, 1:ATAPI.
                uint32_t CommandSets;  // Command Sets Supported.
                uint32_t Size;         // Size in Sectors.
                char     Model[41];    // Model in string.
            };

            class IDEController : public Driver, public DiskController
            {
            private:
                // To which pci device is this IDE controller connected?
                PCIDevice* pciDevice = 0;

                // Array of all interrupt handlers (maximum 2)
                IDEInterruptHandler* interruptHandlers[2] = {0,0};

                // Channels present on this IDE Controller
                IDEChannel channels[2];

                // Connected devices to this controller
                List<IDEDevice*> devices = List<IDEDevice*>();
            private:
                uint8_t ReadRegister(uint8_t channel, uint8_t reg);
                void WriteRegister(uint8_t channel, uint8_t reg, uint8_t data);
            public:
                // Construct new class in memory and pass though connected PCI device
                IDEController(PCIDevice* device);

                // Called from IDEInterrupt handler class
                void HandleIRQ(uint32_t esp);

                // Initialize IDE controller and setup connected devices
                bool Initialize() override;

                // DiskController Functions
                char ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf) override;
                char WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf) override;
                bool EjectDrive(uint8_t drive) override;
            };
        }
    }
}

#endif