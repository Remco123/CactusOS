#ifndef __CACTUSOS__SYSTEM__DISKS__CONTROLLERS__FDC_H
#define __CACTUSOS__SYSTEM__DISKS__CONTROLLERS__FDC_H

#include <core/port.h>
#include <core/pit.h>
#include <core/dma.h>

#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        /* Controller I/O ports */
        enum FLPYDSK_IO {
                FLPYDSK_DOR		                                    =	0x3f2,
                FLPYDSK_MSR		                                    =	0x3f4,
                FLPYDSK_FIFO	                                    =	0x3f5,
                FLPYDSK_CTRL	                                    =	0x3f7
        };

        /* b0-4 of command */
        enum FLPYDSK_CMD {
                FDC_CMD_READ_TRACK		                            =	2,
                FDC_CMD_SPECIFY			                            =	3,
                FDC_CMD_CHECK_STAT		                            =	4,
                FDC_CMD_WRITE_SECT		                            =	5,
                FDC_CMD_READ_SECT		                            =	6,
                FDC_CMD_CALIBRATE		                            =	7,
                FDC_CMD_CHECK_INT		                            =	8,
                FDC_CMD_FORMAT_TRACK	                            =	0xd,
                FDC_CMD_SEEK			                            =	0xf
        };

        enum FLPYDSK_CMD_EXT {

                FDC_CMD_EXT_SKIP                                    =   0x20,    //00100000
                FDC_CMD_EXT_DENSITY                                 =   0x40,    //01000000
                FDC_CMD_EXT_MULTITRACK                              =   0x80     //10000000
        };

        /* digital output */

        enum FLPYDSK_DOR_MASK {

                FLPYDSK_DOR_MASK_DRIVE0                             =        0,        //00000000        = here for completeness sake
                FLPYDSK_DOR_MASK_DRIVE1                             =        1,        //00000001
                FLPYDSK_DOR_MASK_DRIVE2                             =        2,        //00000010
                FLPYDSK_DOR_MASK_DRIVE3                             =        3,        //00000011
                FLPYDSK_DOR_MASK_RESET                              =        4,        //00000100
                FLPYDSK_DOR_MASK_DMA                                =        8,        //00001000
                FLPYDSK_DOR_MASK_DRIVE0_MOTOR                       =        16,       //00010000
                FLPYDSK_DOR_MASK_DRIVE1_MOTOR                       =        32,       //00100000
                FLPYDSK_DOR_MASK_DRIVE2_MOTOR                       =        64,       //01000000
                FLPYDSK_DOR_MASK_DRIVE3_MOTOR                       =        128       //10000000
        };

        /* main status */


        enum FLPYDSK_MSR_MASK {

                FLPYDSK_MSR_MASK_DRIVE1_POS_MODE                =        1,         //00000001
                FLPYDSK_MSR_MASK_DRIVE2_POS_MODE                =        2,         //00000010
                FLPYDSK_MSR_MASK_DRIVE3_POS_MODE                =        4,         //00000100
                FLPYDSK_MSR_MASK_DRIVE4_POS_MODE                =        8,         //00001000
                FLPYDSK_MSR_MASK_BUSY                           =        16,        //00010000
                FLPYDSK_MSR_MASK_DMA                            =        32,        //00100000
                FLPYDSK_MSR_MASK_DATAIO                         =        64,        //01000000
                FLPYDSK_MSR_MASK_DATAREG                        =        128        //10000000
        };

        /* controller status port 0 */
        enum FLPYDSK_ST0_MASK {

                FLPYDSK_ST0_MASK_DRIVE0                 =        0,                //00000000
                FLPYDSK_ST0_MASK_DRIVE1                 =        1,                //00000001
                FLPYDSK_ST0_MASK_DRIVE2                 =        2,                //00000010
                FLPYDSK_ST0_MASK_DRIVE3                 =        3,                //00000011
                FLPYDSK_ST0_MASK_HEADACTIVE             =        4,                //00000100
                FLPYDSK_ST0_MASK_NOTREADY               =        8,                //00001000
                FLPYDSK_ST0_MASK_UNITCHECK              =        16,               //00010000
                FLPYDSK_ST0_MASK_SEEKEND                =        32,               //00100000
                FLPYDSK_ST0_MASK_INTCODE                =        64                //11000000
        };

        /* INTCODE types */
        enum FLPYDSK_ST0_INTCODE_TYP {

                FLPYDSK_ST0_TYP_NORMAL                  =        0,
                FLPYDSK_ST0_TYP_ABNORMAL_ERR            =        1,
                FLPYDSK_ST0_TYP_INVALID_ERR             =        2,
                FLPYDSK_ST0_TYP_NOTREADY                =        3
        };

        /* third gap sizes */
        enum FLPYDSK_GAP3_LENGTH {

                FLPYDSK_GAP3_LENGTH_STD     = 42,
                FLPYDSK_GAP3_LENGTH_5_14    = 32,
                FLPYDSK_GAP3_LENGTH_3_5     = 27
        };

        /* DTL size */
        enum FLPYDSK_SECTOR_DTL {

                FLPYDSK_SECTOR_DTL_128        =        0,
                FLPYDSK_SECTOR_DTL_256        =        1,
                FLPYDSK_SECTOR_DTL_512        =        2,
                FLPYDSK_SECTOR_DTL_1024       =        4
        };

        static char * drive_types[8] = {
            "none",
            "360kB 5.25\"",
            "1.2MB 5.25\"",
            "720kB 3.5\"",

            "1.44MB 3.5\"",
            "2.88MB 3.5\"",
            "unknown type",
            "unknown type"
        };

        #define FDC_DMA_CHANNEL 2
        #define FLPY_SECTORS_PER_TRACK 18
        #define DMA_BUFFER 0x1000

        class FloppyDiskController : public DiskController, public core::InterruptHandler
        {
        private:
            struct FloppyDisk
            {
                bool Present;
                common::uint8_t type;
            } FloppyDisks[2];

            common::uint8_t floppy_irq = 0;

            void DetectDrives();

            common::uint8_t GetStatus();
            void WriteDOR(common::uint8_t cmd);
            void WriteCMD(common::uint8_t cmd);
            common::uint8_t ReadData();
            void WriteCCR(common::uint8_t cmd);
            void DisableController();
            void EnableController();

            void InitDMA(common::uint8_t* buffer, common::uint32_t length);
            void CheckInt(common::uint32_t* st0, common::uint32_t* cy1);
            common::uint8_t WaitForIRQ();
            void SetMotor(common::uint8_t drive, common::uint8_t status);
            void DriveSet(common::uint8_t step, common::uint8_t loadt, common::uint8_t unloadt, common::uint8_t dma);
            void Calibrate(common::uint8_t drive);
            common::uint8_t Seek(common::uint8_t cyl, common::uint8_t head);
            common::uint8_t flpy_read_sector(common::uint8_t head, common::uint8_t track, common::uint8_t sector);
            void LbaToChs(int lba, int* head, int* track, int* sector);
            common::uint8_t* ReadLBA(int lba);
            common::uint8_t WriteLBA(common::uint8_t* buf, common::uint32_t lba);

            void Reset();
        public:
            FloppyDiskController(core::InterruptManager* interrupts);

            common::uint32_t HandleInterrupt(common::uint32_t esp);

            void InitController();
            char ReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            char WriteSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf);
            void AsignDisks(DiskManager* manager);
        };
    }
}

#endif