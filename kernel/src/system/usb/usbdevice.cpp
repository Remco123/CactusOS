#include <system/log.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/usb/usbdevice.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

char* USBControllerStrings[] =
{
    "UHCI",
    "OHCI",
    "EHCI",
    "xHCI"
};

//Create new USBDevice, only called by controllers
USBDevice::USBDevice()
{ }

//Automaticly test this device for its specs and assign a driver if found
bool USBDevice::AssignDriver()
{
    if(this->controller == 0 || this->devAddress == 0) {
        Log(Error, "USB Device not properly initialized by controller");
        return false;
    }
    Log(Info, "Assigning driver for USBDevice %d on %s Controller", this->portNum, USBControllerStrings[this->controller->type]);
    
    struct DEVICE_DESC dev_desc;
    MemoryOperations::memset(&dev_desc, 0, sizeof(struct DEVICE_DESC));
    if(!this->controller->GetDeviceDescriptor(&dev_desc, this))
        return false;
    
    Log(Info, "USBDevice Descriptor:"
        "\n                 len: %d"
        "\n                type: %d"
        "\n             version: %b.%b"
        "\n               class: %d"
        "\n            subclass: %d"
        "\n            protocol: %d"
        "\n     max packet size: %d"
        "\n           vendor id: %w"
        "\n          product id: %w"
        "\n         release ver: %d%d.%d%d"
        "\n   manufacture index: %d (index to a string)"
        "\n       product index: %d"
        "\n        serial index: %d"
        "\n   number of configs: %d",
        dev_desc.len, dev_desc.type, dev_desc.usb_ver >> 8, dev_desc.usb_ver & 0xFF, dev_desc._class, dev_desc.subclass, 
        dev_desc.protocol, dev_desc.max_packet_size, dev_desc.vendorid, dev_desc.productid, 
        (dev_desc.device_rel & 0xF000) >> 12, (dev_desc.device_rel & 0x0F00) >> 8,
        (dev_desc.device_rel & 0x00F0) >> 4,  (dev_desc.device_rel & 0x000F) >> 0,
        dev_desc.manuf_indx, dev_desc.prod_indx, dev_desc.serial_indx, dev_desc.configs);
    
    struct STRING_DESC stringLangDesc;
    MemoryOperations::memset(&stringLangDesc, 0, sizeof(struct STRING_DESC));
    if(!this->controller->GetStringDescriptor(&stringLangDesc, this, 0))
        return false;
    
    //Check if device supports some languages
    if(stringLangDesc.len > 2)
    {
        int englishIndex = -1; //Index of english LANGID
        Log(Info, "Supported Languages by USB Device = %d", (stringLangDesc.len - 2) / 2);
        for(int i = 0; i < (stringLangDesc.len - 2) / 2; i++) {
            Log(Info, "Language: %w", stringLangDesc.string[i]);
            if(stringLangDesc.string[i] == 0x0409)
                englishIndex = i;
        }
        if(englishIndex != -1) //We have found a english LANGID
        {
            //Request device strings in english language
            if(dev_desc.prod_indx != 0) //We have a product string
            {
                struct STRING_DESC stringDesc;
                MemoryOperations::memset(&stringDesc, 0, sizeof(struct STRING_DESC));
                if(this->controller->GetStringDescriptor(&stringDesc, this, dev_desc.prod_indx, 0x0409))
                {
                    Log(Info, "Device String %d", (stringDesc.len-2)/2);
                    for(int i = 0; i < (stringDesc.len-2)/2; i++)
                        BootConsole::Write((char)stringDesc.string[i]);
                    BootConsole::WriteLine();
                }
            }
        }
    }

    return true;
}