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
            #define EHCI_CAPS_CapLength      0x00
            #define EHCI_CAPS_Reserved       0x01
            #define EHCI_CAPS_IVersion       0x02
            #define EHCI_CAPS_HCSParams      0x04
            #define EHCI_CAPS_HCCParams      0x08
            #define EHCI_CAPS_HCSPPortRoute  0x0C

            #define EHCI_OPS_USBCommand       0x00
            #define EHCI_OPS_USBStatus        0x04
            #define EHCI_OPS_USBInterrupt     0x08
            #define EHCI_OPS_FrameIndex       0x0C
            #define EHCI_OPS_CtrlDSSegemnt    0x10
            #define EHCI_OPS_PeriodicListBase 0x14
            #define EHCI_OPS_AsyncListBase    0x18
            #define EHCI_OPS_ConfigFlag       0x40
            #define EHCI_OPS_PortStatus       0x44  // first port

            #define EHCI_PORT_CCS            (1<<0)
            #define EHCI_PORT_CSC            (1<<1)
            #define EHCI_PORT_ENABLED        (1<<2)
            #define EHCI_PORT_ENABLE_C       (1<<3)
            #define EHCI_PORT_OVER_CUR_C     (1<<5)
            #define EHCI_PORT_RESET          (1<<8)
            #define EHCI_PORT_LINE_STATUS    (3<<10)
            #define EHCI_PORT_PP             (1<<12)
            #define EHCI_PORT_OWNER          (1<<13)

            #define EHCI_LEGACY_USBLEGSUP     0x00
            #define EHCI_LEGACY_USBLEGCTLSTS  0x04

            #define EHCI_LEGACY_TIMEOUT     10  // 10 milliseconds
            #define EHCI_LEGACY_BIOS_OWNED  (1<<16)
            #define EHCI_LEGACY_OS_OWNED    (1<<24)
            #define EHCI_LEGACY_OWNED_MASK  (EHC_LEGACY_BIOS_OWNED | EHC_LEGACY_OS_OWNED)

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

            #define E_QUEUE_Q128      0
            #define E_QUEUE_Q64       1
            #define E_QUEUE_Q32       2
            #define E_QUEUE_Q16       3
            #define E_QUEUE_Q8        4
            #define E_QUEUE_Q4        5
            #define E_QUEUE_Q2        6
            #define E_QUEUE_Q1        7

            #define NUM_EHCI_QUEUES 8

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
            } __attribute__((packed)) e_TransferDescriptor_t;

            typedef struct
            {
                uint32_t horzPointer;
                uint32_t flags;
                uint32_t hubFlags;
                uint32_t curQTD;
                e_TransferDescriptor_t transferDescriptor;

                // Additional items
                uint32_t prevPointer;
                uint32_t prevPointerVirt;
                uint32_t horzPointerVirt;
                uint32_t pad[1];
            } __attribute__((packed)) e_queueHead_t;

            class EHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice*      pciDevice;
                uint32_t        regBase = 0;
                uint8_t         operRegsOffset = 0;

                uint32_t        AsyncListVirt = 0;
                uint32_t        AsyncListPhys = 0;

                uint32_t*       frameList = 0;
                uint32_t        frameListPhys = 0;
                e_queueHead_t*  queueStackList = 0;

                uint8_t         dev_address = 1;
                uint8_t         numPorts = 0;

                uint32_t ReadOpReg(uint32_t reg);
                void WriteOpReg(uint32_t reg, uint32_t val);
            public:
                EHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                // Reset port of this controller, returns true when succesfull
                bool ResetPort(uint8_t port);

                uint32_t HandleInterrupt(uint32_t esp);
                
                bool WaitForRegister(const uint32_t reg, const uint32_t mask, const uint32_t result, unsigned ms);
                bool SetupNewDevice(const int port);
                bool EnableAsycnList();
                bool EnablePeriodicList();

                void SetupQueueHead(e_queueHead_t* head, const uint32_t qtd, uint8_t endpt, const uint16_t mps, const uint8_t address);
                int MakeSetupTransferDesc(e_TransferDescriptor_t* tdVirt, const uint32_t tdPhys, uint32_t bufPhys);
                int MakeTransferDesc(uint32_t virtAddr, uint32_t physAddr, const uint32_t status_qtdVirt, const uint32_t status_qtdPhys, uint32_t bufferPhys, const uint32_t size, const bool last, uint8_t data0, const uint8_t dir, const uint16_t mps);
                
                void InsertIntoQueue(e_queueHead_t* item, uint32_t itemPhys, const uint8_t type);
                bool RemoveFromQueue(e_queueHead_t* item);
                
                int WaitForTransferComplete(e_TransferDescriptor_t* td, const uint32_t timeout, bool* spd);
                
                bool ControlOut(const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                

                //////////
                // USB Controller Common Functions
                //////////

                // Function that will get called on a periodic interval in which the controller can perform specific kinds of things.
                void ControllerChecksThread() override;

                // Perform a bulk in operation
                bool BulkIn(USBDevice* device, void* retBuffer, int len, int endP) override;
                // Perform a bulk out operation
                bool BulkOut(USBDevice* device, void* sendBuffer, int len, int endP) override;

                // Perform a control in operation
                bool ControlIn(USBDevice* device, void* target = 0, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0) override;
                // Perform a control out operation
                bool ControlOut(USBDevice* device, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0) override;
            
                // Place a interrupt in transfer in the dedicated queue, handler will get called on completion
                void InterruptIn(USBDevice* device, int len, int endP) override;
            };
        }
    }
}

#endif