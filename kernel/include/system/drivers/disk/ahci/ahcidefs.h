#ifndef __CACTUSOS__SYSTEM__DRIVERS__AHCIDEFS_H
#define __CACTUSOS__SYSTEM__DRIVERS__AHCIDEFS_H

#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>

#include <system/disks/diskcontroller.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            /////////////////////
            // AHCI Port registers
            /////////////////////

            #define AHCI_PORTREG_CMDLISTBASE    0x00    // 0x00, command list base address, 1K-byte aligned               
            #define AHCI_PORTREG_CMDLISTBASE2   0x04    // 0x04, command list base address upper 32 bits              
            #define AHCI_PORTREG_FISLISTBASE    0x08    // 0x08, FIS base address, 256-byte aligned               
            #define AHCI_PORTREG_FISLISTBASE2   0x0C    // 0x0C, FIS base address upper 32 bits              
            #define AHCI_PORTREG_INTSTATUS      0x10    // 0x10, interrupt status         
            #define AHCI_PORTREG_INTENABLE      0x14    // 0x14, interrupt enable         
            #define AHCI_PORTREG_CMDANDSTATUS   0x18    // 0x18, command and status              
            #define AHCI_PORTREG_RESERVED0      0x1C    // 0x1C, Reserved         
            #define AHCI_PORTREG_TASKFILE       0x20    // 0x20, task file data          
            #define AHCI_PORTREG_SIGNATURE      0x24    // 0x24, signature         
            #define AHCI_PORTREG_SATASTATUS     0x28    // 0x28, SATA status (SCR0:SStatus)            
            #define AHCI_PORTREG_SATACTRL       0x2C    // 0x2C, SATA control (SCR2:SControl)          
            #define AHCI_PORTREG_SATAERROR      0x30    // 0x30, SATA error (SCR1:SError)         
            #define AHCI_PORTREG_SATAACTIVE     0x34    // 0x34, SATA active (SCR3:SActive)            
            #define AHCI_PORTREG_COMMANDISSUE   0x38    // 0x38, command issue              
            #define AHCI_PORTREG_SATANOTIFY     0x3C    // 0x3C, SATA notification (SCR4:SNotification)            
            #define AHCI_PORTREG_FISSWITCHCTRL  0x40    // 0x40, FIS-based switch control
            #define AHCI_PORTREG_SIZE           0x80    // Size of port registers in total             

            /////////////////////
            // AHCI General registers
            /////////////////////

            #define AHCI_REG_HOSTCAP            0x00    // 0x00, Host capability     
            #define AHCI_REG_GLOBALCONTROL      0x04    // 0x04, Global host control    
            #define AHCI_REG_INTSTATUS          0x08    // 0x08, Interrupt status    
            #define AHCI_REG_PORTIMPLEMENTED    0x0C    // 0x0C, Port implemented    
            #define AHCI_REG_VERSION            0x10    // 0x10, Version    
            #define AHCI_REG_CCC_CTL            0x14    // 0x14, Command completion coalescing control    
            #define AHCI_REG_CCC_PTS            0x18    // 0x18, Command completion coalescing ports    
            #define AHCI_REG_ENCLOSURE_LOC      0x1C    // 0x1C, Enclosure management location    
            #define AHCI_REG_ENCLOSURE_CTRL     0x20    // 0x20, Enclosure management control    
            #define AHCI_REG_HOSTCAP2           0x24    // 0x24, Host capabilities extended    
            #define AHCI_REG_BIOSHANDOFF        0x28    // 0x28, BIOS/OS handoff control and status    
            #define AHCI_REG_PORTBASE           0x100   // 0x100 - 0x10FF, Port control registers

            #define HBA_PORT_DET_MASK		0x0000000f	// Device Detection
            #define SSTS_PORT_DET_NODEV		0x00000000	// no device detected
            #define SSTS_PORT_DET_NOPHY		0x00000001	// device present but PHY not est.
            #define SSTS_PORT_DET_PRESENT	0x00000003	// device present and PHY est.
            #define SSTS_PORT_DET_OFFLINE	0x00000004	// device offline due to disabled
            #define SCTL_PORT_DET_NOINIT	0x00000000	// no initalization request
            #define SCTL_PORT_DET_INIT		0x00000001	// perform interface initalization
            #define SCTL_PORT_DET_DISABLE	0x00000004	// disable phy

            // Serial ATA Status and control
            #define HBA_PORT_IPM_MASK		0x00000f00	// Interface Power Management
            #define SSTS_PORT_IPM_ACTIVE	0x00000100	// active state
            #define SSTS_PORT_IPM_PARTIAL	0x00000200	// partial state
            #define SSTS_PORT_IPM_SLUMBER	0x00000600	// slumber power management state
            #define SSTS_PORT_IPM_DEVSLEEP	0x00000800	// devsleep power management state
            #define SCTL_PORT_IPM_NORES		0x00000000	// no power restrictions
            #define SCTL_PORT_IPM_NOPART	0x00000100	// no transitions to partial
            #define SCTL_PORT_IPM_NOSLUM	0x00000200	// no transitions to slumber

            #define	SATA_SIG_ATA	0x00000101	// SATA drive
            #define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
            #define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
            #define	SATA_SIG_PM	    0x96690101	// Port multiplier

            #define AHCI_DEV_NULL 0
            #define AHCI_DEV_SATA 1
            #define AHCI_DEV_SEMB 2
            #define AHCI_DEV_PM 3
            #define AHCI_DEV_SATAPI 4

            #define AHCI_PORT_DET_NODEV		0x00000000	// no device detected
            #define AHCI_PORT_DET_NOPHY		0x00000001	// device present but PHY not est.
            #define AHCI_PORT_DET_PRESENT	0x00000003	// device present and PHY est.
            #define AHCI_PORT_DET_OFFLINE	0x00000004	// device offline due to disabled
            #define AHCI_PORT_DET_NOINIT	0x00000000	// no initalization request
            #define AHCI_PORT_DET_INIT		0x00000001	// perform interface initalization
            #define AHCI_PORT_DET_DISABLE	0x00000004	// disable phy

            #define HBA_PORT_IPM_ACTIVE 1
        
            enum {
                CAP_S64A		= (1U << 31),	// Supports 64-bit Addressing
                CAP_SNCQ		= (1 << 30),	// Supports Native Command Queuing
                CAP_SSNTF		= (1 << 29),	// Supports SNotification Register
                CAP_SMPS		= (1 << 28),	// Supports Mechanical Presence Switch
                CAP_SSS			= (1 << 27),	// Supports Staggered Spin-up
                CAP_SALP		= (1 << 26),	// Supports Aggressive Link Power Management
                CAP_SAL			= (1 << 25),	// Supports Activity LED
                CAP_SCLO		= (1 << 24),	// Supports Command List Override
                CAP_ISS_MASK	= 0xf,			// Interface Speed Support
                CAP_ISS_SHIFT	= 20,
                CAP_SNZO		= (1 << 19),	// Supports Non-Zero DMA Offsets
                CAP_SAM			= (1 << 18),	// Supports AHCI mode only
                CAP_SPM			= (1 << 17),	// Supports Port Multiplier
                CAP_FBSS		= (1 << 16),	// FIS-based Switching Supported
                CAP_PMD			= (1 << 15),	// PIO Multiple DRQ Block
                CAP_SSC			= (1 << 14),	// Slumber State Capable
                CAP_PSC			= (1 << 13),	// Partial State Capable
                CAP_NCS_MASK	= 0x1f,			// Number of Command Slots
                                                // (zero-based number)
                CAP_NCS_SHIFT	= 8,
                CAP_CCCS		= (1 << 7),		// Command Completion Coalescing Supported
                CAP_EMS			= (1 << 6),		// Enclosure Management Supported
                CAP_SXS			= (1 << 5),		// Supports External SATA
                CAP_NP_MASK		= 0x1f,			// Number of Ports (zero-based number)
                CAP_NP_SHIFT	= 0,
            };

            enum {
                CAP2_DESO		= (1 << 5),		// DevSleep Entrance from Slumber Only
                CAP2_SADM		= (1 << 4),		// Supports Aggressive Device Sleep
                                                // Management
                CAP2_SDS		= (1 << 3),		// Supports Device Sleep
                CAP2_APST		= (1 << 2),		// Automatic Partial to Slumber Transitions
                CAP2_NVMP		= (1 << 1),		// NVMHCI Present
                CAP2_BOH		= (1 << 0),		// BIOS/OS Handoff
            };

            typedef enum
            {
                FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
                FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
                FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
                FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
                FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
                FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
                FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
                FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
            } FIS_TYPE;

            enum {
                PORT_CMD_ICC_ACTIVE		= (1 << 28),	// Interface Communication control
                PORT_CMD_ICC_SLUMBER	= (6 << 28),	// Interface Communication control
                PORT_CMD_ICC_MASK		= (0xf<<28),	// Interface Communication control
                PORT_CMD_ATAPI	= (1 << 24),	// Device is ATAPI
                PORT_CMD_CR		= (1 << 15),	// Command List Running (DMA active)
                PORT_CMD_FR		= (1 << 14),	// FIS Receive Running
                PORT_CMD_FRE	= (1 << 4),		// FIS Receive Enable
                PORT_CMD_CLO	= (1 << 3),		// Command List Override
                PORT_CMD_POD	= (1 << 2),		// Power On Device
                PORT_CMD_SUD	= (1 << 1),		// Spin-up Device
                PORT_CMD_ST		= (1 << 0),		// Start DMA
            };


            enum {
                PORT_INT_CPD	= (1 << 31),	// Cold Presence Detect Status/Enable
                PORT_INT_TFE	= (1 << 30),	// Task File Error Status/Enable
                PORT_INT_HBF	= (1 << 29),	// Host Bus Fatal Error Status/Enable
                PORT_INT_HBD	= (1 << 28),	// Host Bus Data Error Status/Enable
                PORT_INT_IF		= (1 << 27),	// Interface Fatal Error Status/Enable
                PORT_INT_INF	= (1 << 26),	// Interface Non-fatal Error Status/Enable
                PORT_INT_OF		= (1 << 24),	// Overflow Status/Enable
                PORT_INT_IPM	= (1 << 23),	// Incorrect Port Multiplier Status/Enable
                PORT_INT_PRC	= (1 << 22),	// PhyRdy Change Status/Enable
                PORT_INT_DI		= (1 << 7),		// Device Interlock Status/Enable
                PORT_INT_PC		= (1 << 6),		// Port Change Status/Enable
                PORT_INT_DP		= (1 << 5),		// Descriptor Processed Interrupt
                PORT_INT_UF		= (1 << 4),		// Unknown FIS Interrupt
                PORT_INT_SDB	= (1 << 3),		// Set Device Bits FIS Interrupt
                PORT_INT_DS		= (1 << 2),		// DMA Setup FIS Interrupt
                PORT_INT_PS		= (1 << 1),		// PIO Setup FIS Interrupt
                PORT_INT_DHR	= (1 << 0),		// Device to Host Register FIS Interrupt
            };

            #define PORT_INT_ERROR	(PORT_INT_TFE | PORT_INT_HBF | PORT_INT_HBD \
                                        | PORT_INT_IF | PORT_INT_INF | PORT_INT_OF \
                                        | PORT_INT_IPM | PORT_INT_PRC | PORT_INT_PC \
                                        | PORT_INT_UF)

            #define PORT_INT_MASK	(PORT_INT_ERROR | PORT_INT_DP | PORT_INT_SDB \
                                        | PORT_INT_DS | PORT_INT_PS | PORT_INT_DHR)

            typedef struct {
                uint32_t flags;
                uint32_t byteCount;             // Physical Region Descriptor Byte Count (PRDBC)
                uint32_t cmdTableAddress;       // Command Table Descriptor Base Address
                uint32_t cmdTableAddressHigh;   // Command Table Descriptor Base Address Upper 32-bits
                uint32_t resv[4];               // Reserved
            } __attribute__((packed)) a_commandHeader_t;

            typedef struct
            {
                uint32_t dataBase;		    // Data base address
                uint32_t dataBaseHigh;		// Data base address upper 32 bits
                uint32_t rsv0;		        // Reserved
            
                // DW3
                uint32_t byteCount  : 22;   // Byte count, 4M max
                uint32_t rsv1       : 9;	// Reserved
                uint32_t ioc        : 1;	// Interrupt on completion
            } __attribute__((packed)) a_prdtEntry_t;

            typedef struct
            {
                // 0x00
                uint8_t fis[64];	// Command FIS
            
                // 0x40
                uint8_t cmd[16];	// ATAPI command, 12 or 16 bytes
            
                // 0x50
                uint8_t rsv[48];	// Reserved
            
                // 0x80
                a_prdtEntry_t prdt_entry[1]; // Physical region descriptor table entries, 0 ~ 65535
            } __attribute__((packed)) a_commandTable_t;

            typedef struct
            {
                // DWORD 0
                uint8_t  fis_type;	// FIS_TYPE_REG_H2D
            
                uint8_t  pmport:4;	// Port multiplier
                uint8_t  rsv0:3;	// Reserved
                uint8_t  c:1;		// 1: Command, 0: Control
            
                uint8_t  command;	// Command register
                uint8_t  featurel;	// Feature register, 7:0
            
                // DWORD 1
                uint8_t  lba0;		// LBA low register, 7:0
                uint8_t  lba1;		// LBA mid register, 15:8
                uint8_t  lba2;		// LBA high register, 23:16
                uint8_t  device;		// Device register
            
                // DWORD 2
                uint8_t  lba3;		// LBA register, 31:24
                uint8_t  lba4;		// LBA register, 39:32
                uint8_t  lba5;		// LBA register, 47:40
                uint8_t  featureh;	// Feature register, 15:8
            
                // DWORD 3
                uint8_t  countl;		// Count register, 7:0
                uint8_t  counth;		// Count register, 15:8
                uint8_t  icc;		// Isochronous command completion
                uint8_t  control;	// Control register
            
                // DWORD 4
                uint8_t  rsv1[4];	// Reserved
            } __attribute__((packed)) FIS_REG_H2D;

            typedef struct {
                uint8_t	dmaSetup[0x1c];	    // DMA Setup FIS
                
                uint8_t	res1[0x04];
                
                uint8_t	pioSetup[0x14];	    // PIO Setup FIS
                
                uint8_t	res2[0x0c];
                
                uint8_t	d2hRegister[0x14];	// D2H Register FIS
                
                uint8_t	res3[0x04];
                
                uint8_t	devBits[0x08];	    // Set Device Bits FIS
                uint8_t	unknown[0x40];		// Unknown FIS
                
                uint8_t	res4[0x60];
            }  __attribute__((packed)) a_fis_t;

            #define PRD_TABLE_ENTRY_COUNT 168
            #define COMMAND_LIST_COUNT 32
        }
    }
}

#endif