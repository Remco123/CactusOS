#ifndef __CACTUSOS__SYSTEM__USB__USBDEVICE_H
#define __CACTUSOS__SYSTEM__USB__USBDEVICE_H

#include <system/usb/usbcontroller.h>
#include <system/usb/usbendpoint.h>

namespace CactusOS
{
    namespace system
    {
        class USBController;
        class USBDriver;
        
        // Class describing a common interface to interact with different usb devices
        class USBDevice
        {
        public:
            // Port to which this device is attached to on the controller
            // This is the physical port and not a virtual address of some sort
            // The address is stored in the properties per controller
            uint8_t portNum = 0;
            // Ranging from 0 to 127, should not be 0 after initializing
            uint8_t devAddress = 0;
            // Which controller is this device attached to?
            USBController* controller = 0;
            // The name of this device present in the string descriptor
            char* deviceName = 0;
            // Class code of this device
            uint16_t classID = 0;
            // Sub-Class code of this device
            uint16_t subclassID = 0;
            // Protocol used
            uint16_t protocol = 0;
            // Vendor ID of device
            uint16_t vendorID = 0;
            // Product ID of device
            uint16_t productID = 0;

            // Driver associated with this device, 0 if none found
            USBDriver* driver = 0;

            //// Properties per controller
            struct UHCIProperties
            {
                bool lowSpeedDevice;
                int maxPacketSize;
            } uhciProperties;
            struct OHCIProperties
            {
                int desc_mps;
                bool ls_device;
            } ohciProperties;

            // List of all endpoints of device
            List<USBEndpoint*> endpoints;

            // Pointer to HID descriptor found in interface
            // Only valid for mouse or keyboard
            uint8_t* hidDescriptor = 0;
        public:
            // Create new USBDevice, only called by controllers
            USBDevice();
            // Destructor
            ~USBDevice();

            // Automaticly test this device for its specs and assign a driver if found
            bool AssignDriver();

            // Called when device is unplugged from system
            void OnUnplugged();
        };
    }
}

#endif