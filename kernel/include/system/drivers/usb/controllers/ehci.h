#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__EHCI_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__EHCI_H


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
            #define EHC_CAPS_CapLength      0x00
            #define EHC_CAPS_Reserved       0x01
            #define EHC_CAPS_IVersion       0x02
            #define EHC_CAPS_HCSParams      0x04
            #define EHC_CAPS_HCCParams      0x08
            #define EHC_CAPS_HCSPPortRoute  0x0C

            #define EHC_OPS_USBCommand       0x00
            #define EHC_OPS_USBStatus        0x04
            #define EHC_OPS_USBInterrupt     0x08
            #define EHC_OPS_FrameIndex       0x0C
            #define EHC_OPS_CtrlDSSegemnt    0x10
            #define EHC_OPS_PeriodicListBase 0x14
            #define EHC_OPS_AsyncListBase    0x18
            #define EHC_OPS_ConfigFlag       0x40
            #define EHC_OPS_PortStatus       0x44  // first port

            #define EHCI_PORT_CCS            (1<<0)
            #define EHCI_PORT_CSC            (1<<1)
            #define EHCI_PORT_ENABLED        (1<<2)
            #define EHCI_PORT_ENABLE_C       (1<<3)
            #define EHCI_PORT_OVER_CUR_C     (1<<5)
            #define EHCI_PORT_RESET          (1<<8)
            #define EHCI_PORT_LINE_STATUS    (3<<10)
            #define EHCI_PORT_PP             (1<<12)
            #define EHCI_PORT_OWNER          (1<<13)

            #define EHC_LEGACY_USBLEGSUP     0x00
            #define EHC_LEGACY_USBLEGCTLSTS  0x04

            #define EHC_LEGACY_TIMEOUT     10  // 10 milliseconds
            #define EHC_LEGACY_BIOS_OWNED  (1<<16)
            #define EHC_LEGACY_OS_OWNED    (1<<24)
            #define EHC_LEGACY_OWNED_MASK  (EHC_LEGACY_BIOS_OWNED | EHC_LEGACY_OS_OWNED)

            #define EHCI_PORT_WRITE_MASK     0x007FF1EE

            #define EHCI_QUEUE_HEAD_PTR_MASK  0x1F

            #define QH_HS_T0         (0<<0)  // pointer is valid
            #define QH_HS_T1         (1<<0)  // pointer is not valid

            #define QH_HS_TYPE_ISO   (0<<1)  // ISO
            #define QH_HS_TYPE_QH    (1<<1)  // Queue Head

            #define QH_HS_EPS_FS     (0<<12) // Full speed endpoint
            #define QH_HS_EPS_LS     (1<<12) // Low  speed endpoint
            #define QH_HS_EPS_HS     (2<<12) // High speed endpoint

            #define EHCI_TD_PID_OUT    0
            #define EHCI_TD_PID_IN     1
            #define EHCI_TD_PID_SETUP  2

            typedef struct
            {
                uint32_t nextQTD;
                uint32_t altNextQTD;
                uint32_t flags;
                uint32_t bufPtr0;
                uint32_t bufPtr1;
                uint32_t bufPtr2;
                uint32_t bufPtr3;
                uint32_t bufPtr4;

                uint32_t bufPtr0_Hi;
                uint32_t bufPtr1_Hi;
                uint32_t bufPtr2_Hi;
                uint32_t bufPtr3_Hi;
                uint32_t bufPtr4_Hi;

                uint32_t nextQTDVirt;
                uint32_t altNextQTDVirt;

                uint32_t pad[1];
            } __attribute__((packed)) e_queueTransferDescriptor_t;

            typedef struct
            {
                uint32_t horzPointer;
                uint32_t flags;
                uint32_t hubFlags;
                uint32_t curQTD;
                e_queueTransferDescriptor_t transferDescriptor;

                //Additional items
                uint32_t prevPointer;
                uint32_t prevPointerVirt;
                uint32_t horzPointerVirt;
                uint32_t pad[1];
            } __attribute__((packed)) e_queueHead_t;

            // ISO's
            #define EHCI_ISO_OFF_NEXT_TD_PTR        0  // offset of item within td
            #define EHCI_ISO_OFF_STATUS_0           4
            #define EHCI_ISO_OFF_STATUS_1           8
            #define EHCI_ISO_OFF_STATUS_2          12
            #define EHCI_ISO_OFF_STATUS_3          16
            #define EHCI_ISO_OFF_STATUS_4          20
            #define EHCI_ISO_OFF_STATUS_5          24
            #define EHCI_ISO_OFF_STATUS_6          28
            #define EHCI_ISO_OFF_STATUS_7          32
            #define EHCI_ISO_OFF_BUFFER_0          36
            #define EHCI_ISO_OFF_BUFFER_1          40
            #define EHCI_ISO_OFF_BUFFER_2          44
            #define EHCI_ISO_OFF_BUFFER_3          48
            #define EHCI_ISO_OFF_BUFFER_4          52
            #define EHCI_ISO_OFF_BUFFER_5          56
            #define EHCI_ISO_OFF_BUFFER_6          60

            class EHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;
                uint32_t regBase = 0;
                uint8_t operRegsOffset = 0;

                uint32_t AsyncListVirt = 0;
                uint32_t AsyncListPhys = 0;
                uint8_t dev_address = 1;
                uint8_t numPorts = 0;
                bool hasPortIndicators = false;
            public:
                EHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                uint32_t HandleInterrupt(uint32_t esp);
                
                bool ehciHandshake(const uint32_t reg, const uint32_t mask, const uint32_t result, unsigned ms);
                bool StopLegacy(const uint32_t params);
                bool GetDescriptor(const int port);
                bool EnableAsycnList(const bool enable);

                void SetupQueueHead(e_queueHead_t* head, const uint32_t qtd, uint8_t endpt, const uint16_t mps, const uint8_t address);
                int MakeSetupTransferDesc(e_queueTransferDescriptor_t* tdVirt, const uint32_t tdPhys, uint32_t bufPhys);
                int MakeTransferDesc(uint32_t virtAddr, uint32_t physAddr, const uint32_t status_qtdVirt, const uint32_t status_qtdPhys, uint32_t bufferPhys, const uint32_t size, const bool last, uint8_t data0, const uint8_t dir, const uint16_t mps);
                
                void InsertIntoQueue(e_queueHead_t* item, uint32_t itemPhys, const uint8_t type);
                bool RemoveFromQueue(e_queueHead_t* item);
                
                int WaitForInterrupt(e_queueTransferDescriptor_t* td, const uint32_t timeout, bool* spd);
                
                bool ControlOut(const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                
                void CheckPortChange();
                void SetPortIndicator(uint8_t port, uint8_t color);

                uint32_t ReadOpReg(uint32_t reg);
                void WriteOpReg(uint32_t reg, uint32_t val);

                //////////
                // USB Controller Common Functions
                //////////
                //Reset port of this controller, returns true when succesfull
                bool ResetPort(uint8_t port) override;
                //Receive descriptor from device, returns true when succesfull
                bool GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device) override;
                //Receive descriptor from device, returns true when succesfull
                bool GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang = 0) override;            
                //Get String descriptor of specific device
                //Returns buffer with Configuration header and additional data            
                uint8_t* GetConfigDescriptor(USBDevice* device) override;
                //Set configuration for device
                bool SetConfiguration(USBDevice* device, uint8_t config) override;
            };
        }
    }
}

#endif