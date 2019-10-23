#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__XHCI_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__XHCI_H


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

            // Port_info:
            struct S_XHCI_PORT_INFO {
                uint8_t flags;                // port_info flags below
                uint8_t other_port_num;       // zero based offset to other speed port
                uint8_t offset;               // offset of this port within this protocol
                uint8_t reserved;
            } __attribute__((packed));

            class XHCIController : public USBController, public Driver
            {
            private:
                PCIDevice* pciDevice;
                uint32_t regBase;
                uint8_t opRegOffset;
                uint32_t hccparams1;

                S_XHCI_PORT_INFO port_info[16];
            public:
                XHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                bool ResetPort(const int port);

                void WriteOpReg(const uint32_t offset, const uint32_t val);
                void WriteOpReg64(const uint32_t offset, const uint64_t val);
                uint32_t ReadOpReg(const uint32_t offset);
                uint64_t ReadOpReg64(const uint32_t offset);
            };
        }
    }
}

#endif