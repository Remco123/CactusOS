#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__OHCI_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__OHCI_H


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
            #define OHCRevision          0x00
            #define OHCControl           0x04
            #define OHCCommandStatus     0x08
            #define OHCInterruptStatus   0x0C
            #define OHCInterruptEnable   0x10
            #define OHCInterruptDisable  0x14
            #define OHCHCCA              0x18
            #define OHCPeriodCurrentED   0x1C
            #define OHCControlHeadED     0x20
            #define OHCControlCurrentED  0x24
            #define OHCBulkHeadED        0x28
            #define OHCBulkCurrentED     0x2C
            #define OHCDoneHead          0x30
            #define OHCFmInterval        0x34
            #define OHCFmRemaining       0x38
            #define OHCFmNumber          0x3C
            #define OHCPeriodicStart     0x40
            #define OHCLSThreshold       0x44
            #define OHCRhDescriptorA     0x48
            #define OHCRhDescriptorB     0x4C
            #define OHCRhStatus          0x50
            #define OHCRhPortStatus      0x54

            #define OHCRhDescriptorA_MASK 0xFFFFFBF0

            struct OHCI_ED {
                uint32_t flags;
                uint32_t tailp;
                uint32_t headp;
                uint32_t nexted;
            } __attribute__((packed));

            struct OHCI_TD {
                uint32_t flags;
                uint32_t cbp;
                uint32_t nexttd;
                uint32_t be;
            } __attribute__((packed));


            #define TD_DP_SETUP  0
            #define TD_DP_OUT    1
            #define TD_DP_IN     2
            #define TD_DP_RESV   3

            struct OHCI_HCCA {
                uint32_t HccaInterruptTable[32];
                uint16_t HccaFrameNumber;
                uint16_t HccaPad1;
                uint32_t HccaDoneHead;
                uint8_t  reserved[116];
                uint32_t unknown;
            } __attribute__((packed));

            // we have to place all data in a single struct so that we can pass it back
            //  from local access to physcal memory.
            struct OHCI_FRAME {
                struct OHCI_HCCA hcca;            // 256 bytes
                uint8_t  reserved0[16];             //  16
                struct OHCI_ED   ed_table[31];    // 496
                struct OHCI_ED   control_ed[16];  // 256
                struct OHCI_ED   bulk_ed[16];     // 256
                struct OHCI_TD   our_tds[32];     // 32 * 4 * 4 
                uint8_t  setup[8];                  // 8
                uint8_t  packet[32];                // return packet data space
            } __attribute__((packed));

            class OHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;
                uint32_t regBase; //Base address of registers
                OHCI_HCCA* hcca;
                //Value to store physical address of HCCA
                uint32_t hccaPhys;
            public:
                OHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                void CreateStack(struct OHCI_FRAME* frame, const int mps, int cnt, const bool ls_device, const int address);
                void SetAddress(struct OHCI_FRAME* frame, int dev_address, bool ls_device);
                bool RequestDesc(struct OHCI_FRAME* our_frame, int cnt);

                uint32_t HandleInterrupt(uint32_t esp);

                //////////
                // USB Controller Common Functions
                //////////
                //Reset port of this controller, returns true when succesfull
                bool ResetPort(uint8_t port) override;
                //Receive descriptor from device, returns true when succesfull
                bool GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device) override;
            };
        }
    }
}

#endif