#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__XHCI_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__XHCI_H

/*
 * xHCI Support is not working yet, at the moment it is to much work to support the controller.
 * Right now only the ohci,uhci and ehci are supported.
 * Perhaps in the future this implementation will be made to a working state as well
*/
/*
#include <system/drivers/driver.h>
#include <system/components/pci.h>
#include <system/interruptmanager.h>
#include <system/usb/usbcontroller.h>

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            #define xHC_CAPS_CapLength      0x00
            #define xHC_CAPS_Reserved       0x01
            #define xHC_CAPS_IVersion       0x02
            #define xHC_CAPS_HCSParams1     0x04
            #define xHC_CAPS_HCSParams2     0x08
            #define xHC_CAPS_HCSParams3     0x0C
            #define xHC_CAPS_HCCParams1     0x10
            #define xHC_CAPS_DBOFF          0x14
            #define xHC_CAPS_RTSOFF         0x18
            #define xHC_CAPS_HCCParams2     0x1C

            #define xHC_OPS_USBCommand      0x00
            #define xHC_OPS_USBStatus       0x04
            #define xHC_OPS_USBPageSize     0x08
            #define xHC_OPS_USBDnctrl       0x14
            #define xHC_OPS_USBCrcr         0x18
            #define xHC_OPS_USBDcbaap       0x30
            #define xHC_OPS_USBConfig       0x38

            #define xHC_OPS_USBPortSt       0x400
            #define xHC_Port_PORTSC             0
            #define xHC_Port_PORTPMSC           4
            #define xHC_Port_PORTLI             8
            #define xHC_Port_PORTHLPMC         12

            #define USB_TRHRSI    3   // No more than this between resets for root hubs
            #define USB_TRSTRCY  10   // reset recovery
            #define xHC_PortUSB_CHANGE_BITS  ((1<<17) | (1<<18) | (1<<20) | (1<<21) | (1<<22))

            #define xHCI_PROTO_USB2  0
            #define xHCI_PROTO_USB3  1

            #define xHC_xECP_ID_NONE       0
            #define xHC_xECP_ID_LEGACY     1
            #define xHC_xECP_ID_PROTO      2
            #define xHC_xECP_ID_POWER      3
            #define xHC_xECP_ID_VIRT       4
            #define xHC_xECP_ID_MESS       5
            #define xHC_xECP_ID_LOCAL      6
            #define xHC_xECP_ID_DEBUG     10
            #define xHC_xECP_ID_EXT_MESS  17

            // Port_info flags
            #define xHCI_PROTO_INFO           (1<<0)  // bit 0 set = USB3, else USB2
            #define xHCI_PROTO_HSO            (1<<1)  // bit 1 set = is USB 2 and High Speed Only
            #define xHCI_PROTO_HAS_PAIR       (1<<2)  // bit 2 set = has a corresponding port. (i.e.: is a USB3 and has USB2 port (a must))
                                                    //     clear = does not have a corr. port (i.e.: is a USB2 port and does not have a USB3 port)
            #define xHCI_PROTO_ACTIVE         (1<<3)  // is the active port of the pair.

            #define xHCI_IS_USB3_PORT(x)  ((port_info[(x)].flags & xHCI_PROTO_INFO) == xHCI_PROTO_USB3)
            #define xHCI_IS_USB2_PORT(x)  ((port_info[(x)].flags & xHCI_PROTO_INFO) == xHCI_PROTO_USB2)
            #define xHCI_IS_USB2_HSO(x)   ((port_info[(x)].flags & xHCI_PROTO_HSO) == xHCI_PROTO_HSO)
            #define xHCI_HAS_PAIR(x)      ((port_info[(x)].flags & xHCI_PROTO_HAS_PAIR) == xHCI_PROTO_HAS_PAIR)
            #define xHCI_IS_ACTIVE(x)     ((port_info[(x)].flags & xHCI_PROTO_ACTIVE) == xHCI_PROTO_ACTIVE)

            #define xHC_xECP_LEGACY_TIMEOUT     10  // 10 milliseconds
            #define xHC_xECP_LEGACY_BIOS_OWNED  (1<<16)
            #define xHC_xECP_LEGACY_OS_OWNED    (1<<24)
            #define xHC_xECP_LEGACY_OWNED_MASK  (xHC_xECP_LEGACY_BIOS_OWNED | xHC_xECP_LEGACY_OS_OWNED)
            struct xHC_xECP_LEGACY {
                uint32_t volatile id_next_owner_flags;
                uint32_t volatile cntrl_status;
            } __attribute__((packed));

            struct xHCI_TRB {
                uint64_t param;
                uint16_t status;
                uint16_t command;
            } __attribute__((packed));

            // event ring specification
            struct xHCI_EVENT_SEG_TABLE {
                uint64_t addr;
                uint16_t size;
                uint16_t resv;
            } __attribute__((packed));

            #define xHC_INTERRUPTER_IMAN      0x00
            #define xHC_INTERRUPTER_IMOD      0x04
            #define xHC_INTERRUPTER_TAB_SIZE  0x08
            #define xHC_INTERRUPTER_RESV      0x0C
            #define xHC_INTERRUPTER_ADDRESS   0x10
            #define xHC_INTERRUPTER_DEQUEUE   0x18

            #define TRB_GET_STYPE(x)     (((x) & (0x1F << 16)) >> 16)
            #define TRB_SET_STYPE(x)     (((x) & 0x1F) << 16)
            #define TRB_GET_TYPE(x)      (((x) & (0x3F << 10)) >> 10)
            #define TRB_SET_TYPE(x)      (((x) & 0x3F) << 10)
            #define TRB_GET_COMP_CODE(x) (((x) & (0x7F << 24)) >> 24)
            #define TRB_SET_COMP_CODE(x) (((x) & 0x7F) << 24)
            #define TRB_GET_SLOT(x)      (((x) & (0xFF << 24)) >> 24)
            #define TRB_SET_SLOT(x)      (((x) & 0xFF) << 24)
            #define TRB_GET_TDSIZE(x)    (((x) & (0x1F << 17)) >> 17)
            #define TRB_SET_TDSIZE(x)    (((x) & 0x1F) << 17)
            #define TRB_GET_EP(x)        (((x) & (0x1F << 16)) >> 16)
            #define TRB_SET_EP(x)        (((x) & 0x1F) << 16)

            #define TRB_GET_TARGET(x)    (((x) & (0x3FF << 22)) >> 22)
            #define TRB_GET_TX_LEN(x)     ((x) & 0x1FFFF)
            #define TRB_GET_TOGGLE(x)    (((x) & (1<<1)) >> 1)

            #define TRB_DC(x)            (((x) & (1<<9)) >> 9)
            #define TRB_IS_IMMED_DATA(x) (((x) & (1<<6)) >> 6)
            #define TRB_IOC(x)           (((x) & (1<<5)) >> 5)
            #define TRB_CHAIN(x)         (((x) & (1<<4)) >> 4)
            #define TRB_SPD(x)           (((x) & (1<<2)) >> 2)
            #define TRB_TOGGLE(x)        (((x) & (1<<1)) >> 1)

            #define TRB_CYCLE_ON          (1<<0)
            #define TRB_CYCLE_OFF         (0<<0)

            #define TRB_TOGGLE_CYCLE_ON   (1<<1)
            #define TRB_TOGGLE_CYCLE_OFF  (0<<1)

            #define TRB_CHAIN_ON          (1<<4)
            #define TRB_CHAIN_OFF         (0<<4)

            #define TRB_IOC_ON            (1<<5)
            #define TRB_IOC_OFF           (0<<5)

            #define TRB_LINK_CMND         (TRB_SET_TYPE(LINK) | TRB_IOC_OFF | TRB_CHAIN_OFF | TRB_TOGGLE_CYCLE_OFF | TRB_CYCLE_ON)

            // Common TRB types
            enum { NORMAL=1, SETUP_STAGE, DATA_STAGE, STATUS_STAGE, ISOCH, LINK, EVENT_DATA, NO_OP,
                ENABLE_SLOT=9, DISABLE_SLOT, ADDRESS_DEVICE, CONFIG_EP, EVALUATE_CONTEXT, RESET_EP,
                STOP_EP=15, SET_TR_DEQUEUE, RESET_DEVICE, FORCE_EVENT, DEG_BANDWIDTH, SET_LAT_TOLERANCE,
                GET_PORT_BAND=21, FORCE_HEADER, NO_OP_CMD,  // 24 - 31 = reserved
                TRANS_EVENT=32, COMMAND_COMPLETION, PORT_STATUS_CHANGE, BANDWIDTH_REQUEST, DOORBELL_EVENT,
                HOST_CONTROLLER_EVENT=37, DEVICE_NOTIFICATION, MFINDEX_WRAP, 
                // 40 - 47 = reserved
                // 48 - 63 = Vendor Defined
            };

            // event completion codes
            enum { TRB_SUCCESS=1, DATA_BUFFER_ERROR, BABBLE_DETECTION, TRANSACTION_ERROR, TRB_ERROR, STALL_ERROR,
                RESOURCE_ERROR=7, BANDWIDTH_ERROR, NO_SLOTS_ERROR, INVALID_STREAM_TYPE, SLOT_NOT_ENABLED, EP_NOT_ENABLED,
                SHORT_PACKET=13, RING_UNDERRUN, RUNG_OVERRUN, VF_EVENT_RING_FULL, PARAMETER_ERROR, BANDWITDH_OVERRUN,
                CONTEXT_STATE_ERROR=19, NO_PING_RESPONSE, EVENT_RING_FULL, INCOMPATIBLE_DEVICE, MISSED_SERVICE,
                COMMAND_RING_STOPPED=24, COMMAND_ABORTED, STOPPED, STOPPER_LENGTH_ERROR, RESERVED, ISOCH_BUFFER_OVERRUN,
                EVERN_LOST=32, UNDEFINED, INVALID_STREAM_ID, SECONDARY_BANDWIDTH, SPLIT_TRANSACTION
                // 37 - 191 reserved
                // 192 - 223 vender defined errors
                // 224 - 225 vendor defined info
            };

            #define CMND_RING_TRBS   128  // not more than 4096

            #define TRBS_PER_RING    256

            #define xHCI_SPEED_FULL   1
            #define xHCI_SPEED_LOW    2
            #define xHCI_SPEED_HI     3
            #define xHCI_SPEED_SUPER  4
            #define XHCI_IRQ_DONE  (1<<31)

            // Port_info:
            struct S_XHCI_PORT_INFO {
                uint8_t flags;                // port_info flags below
                uint8_t other_port_num;       // zero based offset to other speed port
                uint8_t offset;               // offset of this port within this protocol
                uint8_t reserved;
            } __attribute__((packed));

            class XHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;
                uint32_t regBase;
                uint8_t opRegOffset;

                //Controller Registers
                uint32_t hccparams1, hccparams2, hcsparams1, hcsparams2;
                uint32_t rts_offset, db_offset; 
                uint32_t context_size;
                uint32_t pageSize;

                //Memory Structures
                uint32_t devContextAreaPhys, devContextAreaVirt;
                uint32_t commandRingPhys, commandRingVirt;
                uint32_t commandTrbPhys, commandTrbVirt;
                uint32_t commandTrbCycle;
                uint32_t curEventRingAddrPhys, curEventRingAddrVirt;
                uint32_t curEventRingCycle;

                //Port info
                uint8_t numPorts;
                S_XHCI_PORT_INFO port_info[16];
            public:
                XHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                bool ResetPort(const int port);
                bool GetDescriptor(const int port);
                uint32_t HandleInterrupt(uint32_t esp);

                bool StopLegacy(uint32_t list_off);
                uint32_t GetProtoOffset(uint32_t list_off, const int version, int* offset, int* count, uint16_t* flags);
                uint32_t CreateRing(const int trbs);
                uint32_t CreateEventRing(const int trbs, uint32_t* ret_addr);
                bool SendCommand(struct xHCI_TRB* trb, const bool ring_it);
                void GetTrb(struct xHCI_TRB* trb, const uint32_t address);
                void SetTrb(struct xHCI_TRB* trb, const uint32_t address);

                void WriteDoorbell(const uint32_t slot_id, const uint32_t val);
                void WriteOpReg(const uint32_t offset, const uint32_t val);
                void WriteOpReg64(const uint32_t offset, const uint64_t val);
                uint32_t ReadOpReg(const uint32_t offset);
                uint64_t ReadOpReg64(const uint32_t offset);
                void writePrimaryIntr(const uint32_t offset, const uint32_t val);
                void writePrimaryIntr64(const uint32_t offset, const uint64_t val);
                uint32_t readPrimaryIntr(const uint32_t offset);
                uint64_t readPrimaryIntr64(const uint32_t offset);
            };
        }
    }
}
*/
#endif