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

            #define IDE_CTRL_IE  (0<<1)
            #define IDE_CTRL_ID  (1<<1)

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
            #define ATA_CMD_SETFEATURE        0xEF

            #define ATA_FEATURE_TRANSFER_MODE 0x03

            #define ATAPI_CMD_READ           0xA8
            #define ATAPI_CMD_EJECT          0x1B

            #define ATA_IDENT_CAPABILITIES 49
            #define ATA_IDENT_MODEL        54
            #define ATA_IDENT_MAX_LBA      60
            #define ATA_IDENT_COMMANDSETS  83
            #define ATA_IDENT_MAX_LBA_EXT  100

            #define IDE_ATA        0x00
            #define IDE_ATAPI      0x01
            
            #define ATA_MASTER     0x00
            #define ATA_SLAVE      0x01

            #define ATA_SECTOR_SIZE     512
            #define ATAPI_SECTOR_SIZE   2048

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

            // Busmaster stuff
            #define IDE_REG_BMI_CMD    0x0E
            #define IDE_REG_BMI_STS    0x10
            #define IDE_REG_BMI_PRDT   0x12

            #define IDE_CHANNEL_PRIMARY     0x00
            #define IDE_CHANNEL_SECONDARY   0x01

            #define IDE_TIMEOUT 1000
            #define IDE_LOG Log(Info, "IDE %s On line %d", __FILE__, __LINE__);

            struct IDEPhysRegionDescriptor
            {
                uint32_t bufferPtrPhys;
                uint16_t byteCount;
                uint16_t flags; 
            } __attribute__((packed));

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
                uint16_t bmideReg;    // Bus Master IDE
            };

            // Holds information about each IDE-Device
            struct IDEDevice {
                uint8_t  Channel;      // 0 (Primary Channel) or 1 (Secondary Channel).
                uint8_t  Drive;        // 0 (Master Drive) or 1 (Slave Drive).
                uint16_t Type;         // 0: ATA, 1:ATAPI.
                uint32_t Size;         // Size in Sectors.
                char     Model[41];    // Model in string.

                struct 
                {
                    bool IO_Ready;         // Does the drive use the ready bit for data transfers?
                    bool use48_Bit;        // Does the drive use 48-bit addressing for LBA
                    bool legacyDMA;        // Are we using legacy DMA for this device MWDMA?
                    int8_t dmaLevel;      // DMA Type of this drive
                } specs;

                IDEPhysRegionDescriptor* prdt;              // Physical Region Descriptor Table for this channel
                uint32_t                 prdtPhys;          // Physical address of PRDT
                uint8_t*                 prdtBuffer;        // Buffer for reading and writing with DMA commands
                uint32_t                 prdtBufferPhys;    // Physical address of memory pointed to by prdtBuffer
            };

            // Does device support DMA Commands?
            #define IDE_DEV_DMA(x) (x->specs.dmaLevel >= 0)

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
                List<IDEDevice*> devices;

                // SCSI Command Packet
                uint8_t atapiPacket[12];

                // Generic mutex for complete controller
                MutexLock ideLock;

                // Flag used for waiting for IRQ
                volatile bool irqState = 0;
            private:
                uint8_t ReadRegister(uint8_t channel, uint8_t reg);
                void WriteRegister(uint8_t channel, uint8_t reg, uint8_t data);
                inline const void Wait400NS(uint8_t channel); // Wait 400 ns

                // Enable or disable interrupt for specific channel
                inline const void SetChannelInterruptEnable(uint8_t channel, bool enable);

                // Enable a specific feature for this device
                inline const void SetDeviceFeature(uint8_t channel, uint8_t feature, uint8_t arg1 = 0, uint8_t arg2 = 0, uint8_t arg3 = 0, uint8_t arg4 = 0);

                // Set IDE registers for a count of sectors and LBA
                inline const void SetCountAndLBA(uint8_t channel, uint16_t count, uint32_t lba, bool extended);

                // Prepare the ATAPI SCSI Packet
                inline const void PrepareSCSI(uint8_t command, uint32_t lba, uint16_t count, bool dma);

                // Perform a polling operation (for PIO commands) and check for errors if needed
                inline const bool Polling(uint8_t channel, bool checkError);

                // Read DATA port into buffer
                inline const void PIOReadData(uint8_t channel, bool withIORDY, uint8_t* buffer, uint32_t bytes);

                // Write DATA port into buffer
                inline const void PIOWriteData(uint8_t channel, bool withIORDY, uint8_t* buffer, uint32_t bytes);

                // Send a SCSI packet to the drive using PIO
                inline const bool SendPacketCommand(uint8_t channel, uint8_t cmd, uint32_t lba, uint16_t count, bool dma, bool iordy);

                // Wait for IRQ to be fired
                inline const void WaitForIRQ();

                bool WaitForClear(uint8_t channel, uint8_t reg, uint8_t bits, uint32_t timeout, bool yield = false); // Wait for register to clear specific bit
                bool WaitForSet(uint8_t channel, uint8_t reg, uint8_t bits, uint32_t timeout, bool yield = false);   // Wait for register to set specific bit
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

                // Read/Write functions for ATA/ATAPI using DMA

                // Transfer sectors via DMA to a ATA device
                char ATA_DMA_TransferSector(uint16_t drive, uint32_t lba, uint8_t* buf, bool read);
                
                // Transfer sectors via DMA to a ATAPI device (only read is supported)
                char ATAPI_DMA_TransferSector(uint16_t drive, uint16_t lba, uint8_t* buf);
            
                // Read/Write functions for ATA/ATAPI using PIO

                // Transfer sectors via PIO to a ATA device
                char ATA_PIO_TransferSector(uint16_t drive, uint32_t lba, uint8_t* buf, bool read);
                
                // Transfer sectors via PIO to a ATAPI device (only read is supported)
                char ATAPI_PIO_TransferSector(uint16_t drive, uint16_t lba, uint8_t* buf);
            };
        }
    }
}

#endif