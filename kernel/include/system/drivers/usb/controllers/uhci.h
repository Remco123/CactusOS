#ifndef __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__UHCI_H
#define __CACTUSOS__SYSTEM__DRIVERS__USB__CONTROLLERS__UHCI_H


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
            #define UHCI_COMMAND     0x00
            #define UHCI_STATUS      0x02

            #define UHCI_INTERRUPT   0x04
            #define UHCI_FRAME_NUM   0x06
            #define UHCI_FRAME_BASE  0x08
            #define UHCI_SOF_MOD     0x0C
            #define UHCI_LEGACY      0xC0

            #define UHCI_PORT_WRITE_MASK  0x124E    //  0001 0010 0100 1110

            #define UHCI_STS_HCHALTED            (1<<5)
            #define UHCI_STS_HC_PROCESS_ERROR    (1<<4)
            #define UHCI_STS_HOST_SYSTEM_ERROR   (1<<3)
            #define UHCI_STS_RESUME_DETECT       (1<<2)
            #define UHCI_STS_USB_ERROR           (1<<1)
            #define UHCI_STS_USBINT              (1<<0)

            #define TOKEN_OUT    0xE1
            #define TOKEN_IN     0x69
            #define TOKEN_SETUP  0x2D

            #define BREADTH (0<<2)
            #define DEPTH   (1<<2)

            #define QUEUE_HEAD_PTR_MASK  0xFFFFFFF0
            #define QUEUE_HEAD_Q         0x00000002
            #define QUEUE_HEAD_T         0x00000001

            typedef struct UHCI_QUEUE_HEAD {
                uint32_t   horz_ptr;
                uint32_t   vert_ptr;
                uint32_t   resv0[2];   // to make it 16 bytes in length
            } u_queueHead_t;


            #define TD_PTR_MASK  0xFFFFFFF0
            #define TD_VF        0x00000004
            #define TD_Q         0x00000002
            #define TD_T         0x00000001

            #define TD_FLAGS_SPD      0x20000000
            #define TD_FLAGS_CERR     0x18000000
            #define TD_FLAGS_LS       0x04000000
            #define TD_FLAGS_ISO      0x02000000
            #define TD_FLAGS_IOC      0x01000000
            #define TD_STATUS_ACTIVE  0x00800000
            #define TD_STATUS_STALL   0x00400000
            #define TD_STATUS_DBERR   0x00200000
            #define TD_STATUS_BABBLE  0x00100000
            #define TD_STATUS_NAK     0x00080000
            #define TD_STATUS_CRC_TO  0x00040000
            #define TD_STATUS_BSTUFF  0x00020000
            #define TD_STATUS_MASK    0x00FF0000
            #define TD_ACTLEN_MASK    0x000007FF

            #define TD_INFO_MAXLEN_MASK   0xFFE00000
            #define TD_INFO_MAXLEN_SHFT   21
            #define TD_INFO_D             0x00080000
            #define TD_INFO_ENDPT_MASK    0x00078000
            #define TD_INFO_ENDPT_SHFT    15
            #define TD_INFO_ADDR_MASK     0x00007F00
            #define TD_INFO_ADDR_SHFT     8
            #define TD_INFO_PID           0x000000FF

            typedef struct UHCI_TRANSFER_DESCRIPTOR { 
                uint32_t   link_ptr;
                uint32_t   reply;
                uint32_t   info;
                uint32_t   buff_ptr;
                uint32_t   resv0[4];          // the last 4 dwords are reserved for software use.
            } u_transferDescriptor_t;

            class UHCIController : public USBController, public Driver, public InterruptHandler
            {
            private:
                PCIDevice* pciDevice;

                uint32_t* frameList = 0;
                uint32_t frameListPhys = 0;
            public:
                UHCIController(PCIDevice* device);

                bool Initialize() override;
                void Setup() override;

                //Check if this port is present on the controller
                bool PortPresent(uint8_t port);

                bool ControlOut(const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                bool ControlIn(void* targ, const bool lsDevice, const int devAddress, const int packetSize, const int len = 0, const uint8_t requestType = 0, const uint8_t request = 0, const uint16_t valueHigh = 0, const uint16_t valueLow = 0, const uint16_t index = 0);
                
                uint32_t HandleInterrupt(uint32_t esp);

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