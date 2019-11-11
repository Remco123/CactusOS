#ifndef __CACTUSOS__SYSTEM__USB__USBDEVICE_H
#define __CACTUSOS__SYSTEM__USB__USBDEVICE_H

#include <system/usb/usbcontroller.h>

namespace CactusOS
{
    namespace system
    {
        class USBController;
        
        //Class describing a common interface to interact with different usb devices
        class USBDevice
        {
        public:
            //Port to which this device is attached to on the controller
            //This is the physical port and not a virtual address of some sort
            //The address is stored in the properties per controller
            uint8_t portNum = 0;
            //Ranging from 0 to 127, should not be 0 after initializing
            uint8_t devAddress = 0;
            //Which controller is this device attached to?
            USBController* controller = 0;
            //The name of this device present in the string descriptor
            char* deviceName = 0;

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
        public:
            //Create new USBDevice, only called by controllers
            USBDevice();

            //Automaticly test this device for its specs and assign a driver if found
            bool AssignDriver();
        };
    }
}

#endif