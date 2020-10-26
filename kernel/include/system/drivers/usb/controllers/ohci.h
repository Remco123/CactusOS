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

            typedef struct OHCI_ED {
                uint32_t flags;
                uint32_t tailp;
                uint32_t headp;
                uint32_t nextED;
            } __attribute__((packed)) o_endpointDescriptor_t;

            typedef struct OHCI_TD {
                uint32_t flags;
                uint32_t curBufPtr;
                uint32_t nextTd;
                uint32_t bufEnd;
            } __attribute__((packed)) o_transferDescriptor_t;


            #define TD_DP_SETUP  0
            #define TD_DP_OUT    1
            #define TD_DP_IN     2
            #define TD_DP_RESV   3

            typedef struct OHCI_HCCA {
                uint32_t HccaInterruptTable[32];
                uint16_t HccaFrameNumber;
                uint16_t HccaPad1;
                uint32_t HccaDoneHead;
                uint8_t  reserved[116];
                uint32_t unknown;
            } __attribute__((packed)) HCCA_t;

            #define NUM_CONTROL_EDS     16
            #define NUM_BULK_EDS        16
            #define NUM_INTERRUPT_EDS   32
            #define OHCI_TD_TIMEOUT     1000

            class OHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;
                int newDeviceAddress = 1;
                
                volatile uint32_t regBase; //Base address of registers
                //Host Controller Communication Area
                HCCA_t* hcca;
                //Value to store physical address of HCCA
                uint32_t hccaPhys;
                //Number of root hub ports
                uint8_t numPorts;

                //Lists
                o_endpointDescriptor_t* controlEndpoints[NUM_CONTROL_EDS];          // Control list endpoint descriptors
                uint32_t                controlEndpointsPhys[NUM_CONTROL_EDS];      // Physical Addresses of Control ED's
                o_endpointDescriptor_t* bulkEndpoints[NUM_BULK_EDS];                // Bulk list endpoint descriptors
                uint32_t                bulkEndpointsPhys[NUM_BULK_EDS];            // Physical Addresses of Bulk ED's
                o_endpointDescriptor_t* interruptEndpoints;                         // Interrupt endpoint descriptors
                                                                                    // Physical addresses are in hcca
            public:
                OHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                // Check if a set of transfer descriptors is done executing
                // Returns:
                // 0 -> No Errors and Done
                // 1 -> Generic Error
                // 2 -> NAK
                // 3 -> Not done yet
                int CheckTransferDone(o_transferDescriptor_t* td, int numTDs);

                // Calculate in which queue the packet should be placed for this interval
                int CalculateRequiredQueue(int interval);
                
                //Reset port of this controller, returns true when succesfull
                bool ResetPort(uint8_t port);

                bool ControlOut(const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                
                bool BulkOut(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len = 0);
                bool BulkIn(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len = 0);

                void InterruptIn(const bool lsDevice, const int devAddress, const int packetSize, const int endP, int interval, USBDriver* handler, const int len = 0);

                uint32_t HandleInterrupt(uint32_t esp);

                void SetupNewDevice(uint8_t port);

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