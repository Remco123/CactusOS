#ifndef __CACTUSOS__SYSTEM__DRIVERS__FLOPPY_H
#define __CACTUSOS__SYSTEM__DRIVERS__FLOPPY_H

#include <system/drivers/driver.h>
#include <system/interruptmanager.h>
#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            /*
            **	Controller I/O Ports. Please see chapter for additional ports
            */

            enum FLPYDSK_IO
            {
                FLPYDSK_DOR		=	0x3f2,
                FLPYDSK_MSR		=	0x3f4,
                FLPYDSK_FIFO	=	0x3f5,
                FLPYDSK_CTRL	=	0x3f7
            };

            /**
            *	Bits 0-4 of command byte. Please see chapter for additional commands
            */

            enum FLPYDSK_CMD 
            {
                FDC_CMD_READ_TRACK	=	2,
                FDC_CMD_SPECIFY		=	3,
                FDC_CMD_CHECK_STAT	=	4,
                FDC_CMD_WRITE_SECT	=	5,
                FDC_CMD_READ_SECT	=	6,
                FDC_CMD_CALIBRATE	=	7,
                FDC_CMD_CHECK_INT	=	8,
                FDC_CMD_FORMAT_TRACK=	0xd,
                FDC_CMD_SEEK		=	0xf
            };

            /**
            *	Additional command masks. Can be masked with above commands
            */

            enum FLPYDSK_CMD_EXT 
            {
                FDC_CMD_EXT_SKIP		=	0x20,	//00100000
                FDC_CMD_EXT_DENSITY		=	0x40,	//01000000
                FDC_CMD_EXT_MULTITRACK	=	0x80	//10000000
            };

            /*
            **	Digital Output Register
            */

            enum FLPYDSK_DOR_MASK 
            {
                FLPYDSK_DOR_MASK_DRIVE0			=	0,	//00000000	= here for completeness sake
                FLPYDSK_DOR_MASK_DRIVE1			=	1,	//00000001
                FLPYDSK_DOR_MASK_DRIVE2			=	2,	//00000010
                FLPYDSK_DOR_MASK_DRIVE3			=	3,	//00000011
                FLPYDSK_DOR_MASK_RESET			=	4,	//00000100
                FLPYDSK_DOR_MASK_DMA			=	8,	//00001000
                FLPYDSK_DOR_MASK_DRIVE0_MOTOR	=	16,	//00010000
                FLPYDSK_DOR_MASK_DRIVE1_MOTOR	=	32,	//00100000
                FLPYDSK_DOR_MASK_DRIVE2_MOTOR	=	64,	//01000000
                FLPYDSK_DOR_MASK_DRIVE3_MOTOR	=	128	//10000000
            };

            /**
            *	Main Status Register
            */

            enum FLPYDSK_MSR_MASK 
            {
                FLPYDSK_MSR_MASK_DRIVE1_POS_MODE	=	1,	//00000001
                FLPYDSK_MSR_MASK_DRIVE2_POS_MODE	=	2,	//00000010
                FLPYDSK_MSR_MASK_DRIVE3_POS_MODE	=	4,	//00000100
                FLPYDSK_MSR_MASK_DRIVE4_POS_MODE	=	8,	//00001000
                FLPYDSK_MSR_MASK_BUSY				=	16,	//00010000
                FLPYDSK_MSR_MASK_DMA				=	32,	//00100000
                FLPYDSK_MSR_MASK_DATAIO				=	64, //01000000
                FLPYDSK_MSR_MASK_DATAREG			=	128	//10000000
            };

            /**
            *	Controller Status Port 0
            */

            enum FLPYDSK_ST0_MASK 
            {
                FLPYDSK_ST0_MASK_DRIVE0		=	0,		//00000000	=	for completness sake
                FLPYDSK_ST0_MASK_DRIVE1		=	1,		//00000001
                FLPYDSK_ST0_MASK_DRIVE2		=	2,		//00000010
                FLPYDSK_ST0_MASK_DRIVE3		=	3,		//00000011
                FLPYDSK_ST0_MASK_HEADACTIVE	=	4,		//00000100
                FLPYDSK_ST0_MASK_NOTREADY	=	8,		//00001000
                FLPYDSK_ST0_MASK_UNITCHECK	=	16,		//00010000
                FLPYDSK_ST0_MASK_SEEKEND	=	32,		//00100000
                FLPYDSK_ST0_MASK_INTCODE	=	64		//11000000
            };

            /*
            ** LPYDSK_ST0_MASK_INTCODE types
            */

            enum FLPYDSK_ST0_INTCODE_TYP 
            {
                FLPYDSK_ST0_TYP_NORMAL		=	0,
                FLPYDSK_ST0_TYP_ABNORMAL_ERR=	1,
                FLPYDSK_ST0_TYP_INVALID_ERR	=	2,
                FLPYDSK_ST0_TYP_NOTREADY	=	3
            };

            /**
            *	GAP 3 sizes
            */

            enum FLPYDSK_GAP3_LENGTH 
            {
                FLPYDSK_GAP3_LENGTH_STD     = 42,
                FLPYDSK_GAP3_LENGTH_5_14    = 32,
                FLPYDSK_GAP3_LENGTH_3_5     = 27
            };

            /*
            **	Formula: 2^sector_number * 128, where ^ denotes "to the power of"
            */

            enum FLPYDSK_SECTOR_DTL 
            {
                FLPYDSK_SECTOR_DTL_128	=	0,
                FLPYDSK_SECTOR_DTL_256	=	1,
                FLPYDSK_SECTOR_DTL_512	=	2,
                FLPYDSK_SECTOR_DTL_1024	=	4
            };

            typedef enum {
                FloppyDirectionRead = 1,
                FloppyDirectionWrite = 2
            } FloppyDirection;

            //! sectors per track
            #define FLPY_SECTORS_PER_TRACK 18

            //! FDC uses DMA channel 2
            #define FDC_DMA_CHANNEL 2

            #define BYTES_PER_SECT 512

            // A class that manages the floppies on this system
            class FloppyDriver : public InterruptHandler, public Driver, public DiskController
            {
            private:
                // Used for waiting for command completion
                volatile bool IrqHasFired = false;

                // Buffer used by the DMA Controller, must be bellow 16MB
                uint32_t bufferPhys = 0;

                // Virtual address of buffer
                uint8_t* bufferVirt = 0;

                // Floppies on this controller
                uint8_t flpy1 = 0;
	            uint8_t flpy2 = 0;
            private:
                // Prepare DMA for read/write operation
                bool InitializeDMA(FloppyDirection dir);

                // Wait for a irq to be fired
                void WaitForIRQ();

                // Write val to DOR Register
                void WriteDOR(common::uint8_t val);

                // Write val to Configuation Control Register
                void WriteCCR(common::uint8_t val);

                // Read status register and return value
                common::uint8_t ReadStatus();

                // Get status of interrup register
                void CheckIntStatus(uint32_t* st0, uint32_t* cyl);

                // Send Command to controller
                void SendCommand(common::uint8_t cmd);

                // Read Data from controller
                common::uint8_t ReadData();

                // Turn motor on or off for currentDrive
                void ControlMotor(common::uint8_t drive, bool on);

                // Set drive parameters
                void ConfigureDrive(common::uint8_t stepr, common::uint8_t loadt, common::uint8_t unloadt, bool dma);

                // Calibrate drive
                int Calibrate(common::uint8_t drive);

                // Seek current drive
                int Seek(common::uint8_t drive, common::uint8_t cyl, common::uint8_t head);

                // Disable Controller
                void DisableController();

                // Enable Controller
                void EnableController();

                // Reset Controller
                void ResetController();

                // Core function to read sector
                int TransferSectorCHS(common::uint8_t drive, FloppyDirection dir, common::uint8_t head, common::uint8_t track, common::uint8_t sector);
            public:
                FloppyDriver();

                //////////////
                // Functions for sub-components
                //////////////

                bool Initialize() override;
                char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf) override;
                char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf) override;
                bool EjectDrive(common::uint8_t drive) override;
                common::uint32_t HandleInterrupt(common::uint32_t esp) override;
            };
        }
    }
}

#endif