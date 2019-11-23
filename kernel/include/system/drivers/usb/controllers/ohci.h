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

            #define NUM_CONTROL_EDS 16

            class OHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;
                volatile uint32_t regBase; //Base address of registers
                //Host Controller Communication Area
                HCCA_t* hcca;
                //Value to store physical address of HCCA
                uint32_t hccaPhys;

                //Lists
                o_endpointDescriptor_t* controlEndpoints[NUM_CONTROL_EDS];        //Control list endpoint descriptors
                uint32_t              controlEndpointsPhys[NUM_CONTROL_EDS];    //Physical Addresses of Control ED's
            public:
                OHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                bool ControlOut(const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);

                uint32_t HandleInterrupt(uint32_t esp);
                bool WaitForInterrupt();

                //////////
                // USB Controller Common Functions
                //////////
                //Reset port of this controller, returns true when succesfull
                bool ResetPort(uint8_t port) override;
                //Receive descriptor from device, returns true when succesfull
                bool GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device) override;
                //Receive descriptor from device, returns true when succesfull
                bool GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang) override;
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