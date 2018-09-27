//Thanks to: https://forum.osdev.org/viewtopic.php?f=1&p=167798

#ifndef __CACTUSOS__SYSTEM__DISKS__CONTROLLERS__IDE_H
#define __CACTUSOS__SYSTEM__DISKS__CONTROLLERS__IDE_H

#include <core/port.h>
#include <system/disks/diskcontroller.h>
#include <core/pit.h>

namespace CactusOS
{
    namespace system
    {
        #define      ATA_SR_BSY                 0x80
        #define      ATA_SR_DRDY                0x40
        #define      ATA_SR_DF                  0x20
        #define      ATA_SR_DSC                 0x10
        #define      ATA_SR_DRQ                 0x08
        #define      ATA_SR_CORR                0x04
        #define      ATA_SR_IDX                 0x02
        #define      ATA_SR_ERR                 0x01
        #define      ATA_ER_BBK                 0x80
        #define      ATA_ER_UNC                 0x40
        #define      ATA_ER_MC                  0x20
        #define      ATA_ER_IDNF                0x10
        #define      ATA_ER_MCR                 0x08
        #define      ATA_ER_ABRT                0x04
        #define      ATA_ER_TK0NF               0x02
        #define      ATA_ER_AMNF                0x01

        // ATA-Commands:
        #define      ATA_CMD_READ_PIO           0x20
        #define      ATA_CMD_READ_PIO_EXT       0x24
        #define      ATA_CMD_READ_DMA           0xC8
        #define      ATA_CMD_READ_DMA_EXT       0x25
        #define      ATA_CMD_WRITE_PIO          0x30
        #define      ATA_CMD_WRITE_PIO_EXT      0x34
        #define      ATA_CMD_WRITE_DMA          0xCA
        #define      ATA_CMD_WRITE_DMA_EXT      0x35
        #define      ATA_CMD_CACHE_FLUSH        0xE7
        #define      ATA_CMD_CACHE_FLUSH_EXT    0xEA
        #define      ATA_CMD_PACKET             0xA0
        #define      ATA_CMD_IDENTIFY_PACKET    0xA1
        #define      ATA_CMD_IDENTIFY           0xEC

        #define      ATAPI_CMD_READ             0xA8
        #define      ATAPI_CMD_EJECT            0x1B

        #define      ATA_IDENT_DEVICETYPE       0
        #define      ATA_IDENT_CYLINDERS        2
        #define      ATA_IDENT_HEADS            6
        #define      ATA_IDENT_SECTORS          12
        #define      ATA_IDENT_SERIAL           20
        #define      ATA_IDENT_MODEL            54
        #define      ATA_IDENT_CAPABILITIES     98
        #define      ATA_IDENT_FIELDVALID       106
        #define      ATA_IDENT_MAX_LBA          120
        #define      ATA_IDENT_COMMANDSETS      164
        #define      ATA_IDENT_MAX_LBA_EXT      200

        #define      ATA_MASTER                 0x00
        #define      ATA_SLAVE                  0x01

        #define      IDE_ATA                    0x00
        #define      IDE_ATAPI                  0x01

        // ATA-ATAPI Task-File:
        #define      ATA_REG_DATA               0x00
        #define      ATA_REG_ERROR              0x01
        #define      ATA_REG_FEATURES           0x01
        #define      ATA_REG_SECCOUNT0          0x02
        #define      ATA_REG_LBA0               0x03
        #define      ATA_REG_LBA1               0x04
        #define      ATA_REG_LBA2               0x05
        #define      ATA_REG_HDDEVSEL           0x06
        #define      ATA_REG_COMMAND            0x07
        #define      ATA_REG_STATUS             0x07
        #define      ATA_REG_SECCOUNT1          0x08
        #define      ATA_REG_LBA3               0x09
        #define      ATA_REG_LBA4               0x0A
        #define      ATA_REG_LBA5               0x0B
        #define      ATA_REG_CONTROL            0x0C
        #define      ATA_REG_ALTSTATUS          0x0C
        #define      ATA_REG_DEVADDRESS         0x0D

        // Channels:
        #define      ATA_PRIMARY                0x00
        #define      ATA_SECONDARY              0x01

        // Directions:
        #define      ATA_READ                   0x00
        #define      ATA_WRITE                  0x01

        unsigned static char ide_irq_invoked = 0;
        unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        class IDEController;

        class IDEInterruptHandler : public core::InterruptHandler
        {
        private:
            IDEController* controller;
        public:
            IDEInterruptHandler(IDEController* controller, core::InterruptManager* ints, common::uint32_t number);

            common::uint32_t HandleInterrupt(common::uint32_t esp);
        };


        class IDEController : public DiskController
        {
        private:
            struct channel {
                unsigned short base;  // I/O Base.
                unsigned short ctrl;  // Control Base
                unsigned short bmide; // Bus Master IDE
                unsigned char  nIEN;  // nIEN (No Interrupt);
            } channels[2];

            struct ide_device {
                unsigned char  Reserved;    // 0 (Empty) or 1 (This Drive really exists).
                unsigned char  Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
                unsigned char  Drive;       // 0 (Master Drive) or 1 (Slave Drive).
                unsigned short Type;        // 0: ATA, 1:ATAPI.
                unsigned short Signature;   // Drive Signature
                unsigned short Capabilities;// Features.
                unsigned int   CommandSets; // Command Sets Supported.
                unsigned int   Size;        // Size in Sectors.
                unsigned char  Model[41];   // Model in string.
            } ide_devices[4];

            unsigned char ide_buf[2048] = {0};

            IDEInterruptHandler* intHandle1;
            IDEInterruptHandler* intHandle2;
        protected:
        friend class IDEInterruptHandler;
            void ide_wait_irq();
            void ide_handle_irq();


            unsigned char ide_read(unsigned char channel, unsigned char reg);
            void ide_write(unsigned char channel, unsigned char reg, unsigned char data);
            void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads);
            unsigned char ide_polling(unsigned char channel, unsigned int advanced_check);
            unsigned char ide_print_error(unsigned int drive, unsigned char err);

            unsigned char AtaReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            unsigned char AtaWriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);

            unsigned char ATAPIReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
        public:
            IDEController(core::InterruptManager* interrupts);

            //For basic ussage: InitIDE(0x1F0, 0x3F4, 0x170, 0x374, 0x000);
            void InitIDE(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4, core::PIT* pit);

            char EjectDrive(unsigned char drive);

            char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
        };
    }
}

#endif