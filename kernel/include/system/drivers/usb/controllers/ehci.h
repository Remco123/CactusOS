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
            #define EHCI_OPS_CtrlDSSegment    0x10
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

            #define EHCI_LEGACY_TIMEOUT     100  // 100 milliseconds
            #define EHCI_LEGACY_BIOS_OWNED  (1<<16)
            #define EHCI_LEGACY_OS_OWNED    (1<<24)
            #define EHCI_LEGACY_OWNED_MASK  (EHCI_LEGACY_BIOS_OWNED | EHCI_LEGACY_OS_OWNED)

            #define EHCI_PORT_WRITE_MASK     0x007FF1EE

            #define EHCI_QUEUE_HEAD_PTR_MASK  0x1F

            #define QH_HS_T0         (0<<0)  // pointer is valid
            #define QH_HS_T1         (1<<0)  // pointer is not valid

            #define QH_HS_TYPE_QH    (1<<1)  // Queue Head

            #define QH_HS_EPS_FS     0 // Full speed endpoint
            #define QH_HS_EPS_LS     1 // Low  speed endpoint
            #define QH_HS_EPS_HS     2 // High speed endpoint

            #define EHCI_MPS 64 // Max Packet Speed

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

            typedef struct e_transferDescriptor e_transferDescriptor_t;
            typedef struct e_queueHead e_queueHead_t;

            struct e_transferDescriptor
            {
                uint32_t nextQTD;       // Next Transfer descriptor pointer 31:5 is physical address, 1:4 is reserved, 0 is if the link is valid
                uint32_t altNextQTD;    // Alternative Transfer descriptor on short packet pointer 31:5 is physical address, 1:4 is reserved, 0 is if the link is valid
                uint32_t flags;         // Flags of this transfer descriptor
                uint32_t bufPtr[5];     // Low address of buffer pointer
                uint32_t bufPtrHi[5];   // High address of buffer pointer
                
                // Internal fields
                e_transferDescriptor_t* nextQTDVirt;
                e_transferDescriptor_t* altNextQTDVirt;

                uint32_t pad[1];
            } __attribute__((packed));

            struct e_queueHead
            {
                uint32_t horzPointer;   // Points to next queue head 31:5 is physical address, 4:3 is reserved. 2:1 is type, 0 is if the link is valid
                uint32_t flags;         // Flags of this queue, includes device specific information
                uint32_t hubFlags;      // Flags needed when device is on a external hub
                uint32_t curQTD;        // Current transfer descriptor that is excecuted by the controller
                e_transferDescriptor_t transferDescriptor; // Transfer descriptor overlay

                // Internal fields
                uint32_t prevPointerPhys; // Physical address of previous transfer descriptor
                e_queueHead_t* prevPointerVirt; // Virtual pointer to previous queue head, 0 if invallid
                e_queueHead_t* horzPointerVirt; // Virtual pointer to next queue head, 0 if invallid
                uint32_t pad[1];
            } __attribute__((packed));

            class EHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice*      pciDevice;
                int             newDeviceAddress = 1;
                uint32_t        regBase = 0;
                uint8_t         operRegsOffset = 0;

                e_queueHead_t*  asyncList = 0;
                uint32_t        asyncListPhys = 0;

                uint32_t*       periodicList = 0;
                uint32_t        periodicListPhys = 0;
                e_queueHead_t*  queueStackList = 0;

                uint8_t         numPorts = 0;

                uint32_t ReadOpReg(uint32_t reg);
                void WriteOpReg(uint32_t reg, uint32_t val);
                void DisplayRegisters();
                void PrintTransferDescriptors(e_transferDescriptor_t* td);
                void PrintQueueHead(e_queueHead_t* qh);
                void PrintAsyncQueue();
            public:
                EHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                // Reset port of this controller, returns true when successful
                bool ResetPort(uint8_t port);

                // Reset the controller to its default settings
                bool ResetController();

                // Release BIOS ownership of controller
                // On Entry:
                //   params: the dword value of the capability register
                // On Return:
                //   true if ownership released
                // 
                // Set bit 24 to indicate to the BIOS to release ownership
                // The BIOS should clear bit 16 indicating that it has successfully done so
                // Ownership is released when bit 24 is set *and* bit 16 is clear.
                // This will wait EHC_LEGACY_TIMEOUT ms for the BIOS to release ownership.
                //   (It is unknown the exact time limit that the BIOS has to release ownership.)
                // 
                bool StopLegacy(const uint32_t params);

                void InitializeAsyncList();
                void InitializePeriodicList();

                uint32_t HandleInterrupt(uint32_t esp);
                
                bool WaitForRegister(const uint32_t reg, const uint32_t mask, const uint32_t result, unsigned ms);
                bool SetupNewDevice(const int port);

                void MakeSetupTransferDesc(e_transferDescriptor_t* tdVirt, const uint32_t tdPhys, uint32_t bufPhys);
                void MakeTransferDesc(e_transferDescriptor_t* currentTD, uint32_t physAddr, e_transferDescriptor_t* status_qtd, const uint32_t status_qtdPhys, uint32_t bufferPhys, int size, const bool last, uint8_t data0, const uint8_t dir, const uint16_t mps);
                
                void InsertIntoQueue(e_queueHead_t* item, uint32_t itemPhys, const uint8_t type);
                bool RemoveFromQueue(e_queueHead_t* item);
                
                int WaitForTransferComplete(e_transferDescriptor_t* td, const uint32_t timeout, bool* spd);
                
                bool ControlOut(const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                
                bool BulkOut(USBEndpoint* toggleSrc, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len = 0);
                bool BulkIn(USBEndpoint* toggleSrc, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len = 0);

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