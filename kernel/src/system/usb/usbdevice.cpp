#include <system/log.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/usb/usbdevice.h>
#include <system/drivers/usb/usbdriver.h>

#include <system/drivers/usb/mass_storage.h>
#include <system/drivers/usb/usbmouse.h>
#include <system/drivers/usb/usbkeyboard.h>
#include <system/drivers/usb/usbcomborecv.h>

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

char* USBClassCodeStrings[] = 
{
    "00 Unspecified",
    "01 Audio",
    "02 Communications and CDC Control",
    "03 Human interface device (HID)",
    "05 Physical Interface Device (PID)",
    "06 Image (PTP/MTP)",
    "07 Printer",
    "08 Mass storage (MSC or UMS)",
    "09 USB hub",
    "0A CDC-Data",
    "0B Smart Card",
    "0D Content security",
    "0E Video",
    "0F Personal healthcare device class (PHDC)",
    "10 Audio/Video (AV)",
    "11 Billboard",
    "DC Diagnostic Device",
    "E0 Wireless Controller",
    "EF Miscellaneous",
    "FE Application-specific",
    "FF Vendor-specific"
};
const int numClassCodeStrings = sizeof(USBClassCodeStrings) / sizeof(char*);

//Create new USBDevice, only called by controllers
USBDevice::USBDevice()
: endpoints()
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

    if(dev_desc.len == 0) {
        Log(Error, "No device is connected even though controller says it is");
        return false;
    }
    
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
    
    this->classID = dev_desc._class;
    this->subclassID = dev_desc.subclass;
    this->protocol = dev_desc.protocol;
    this->vendorID = dev_desc.vendorid;
    this->productID = dev_desc.productid;
    
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
                    int strLen = (stringDesc.len-2)/2;
                    if(strLen > 0) {
                        //Convert Unicode string to ASCII
                        this->deviceName = new char[strLen + 1];
                        this->deviceName[strLen] = '\0';
                        for(int i = 0; i < strLen; i++)
                            this->deviceName[i] = stringDesc.string[i];
                    }
                }
            }
        }
    }
    else
        Log(Warning, "USBDevice does not have any language supported");
    
    //Get Config Descriptor
    uint8_t* configBuffer = this->controller->GetConfigDescriptor(this);
    if(configBuffer)
    {
        int confLen = *(uint16_t*)(configBuffer + 2);

        uint8_t* startByte = configBuffer;
        uint8_t* endByte = configBuffer + confLen;

        //Loop through all data
        while ((uint32_t)startByte < (uint32_t)endByte)
        {
            uint8_t length = startByte[0];
            uint8_t type = startByte[1];

            if (length == 9 && type == CONFIG) // CONFIGURATION descriptor
            {
                struct CONFIG_DESC* c = (struct CONFIG_DESC*)startByte;
                Log(Info, "USBDevice Config Desc: NumInterfaces=%d ConfigVal=%d ConfigString=%d Attr=%b MaxPower=%d mA", c->num_interfaces, c->config_val, c->config_indx, c->bm_attrbs, c->max_power);
                
                if(c->config_indx) {
                    struct STRING_DESC configString;
                    MemoryOperations::memset(&configString, 0, sizeof(struct STRING_DESC));
                    if(this->controller->GetStringDescriptor(&configString, this, c->config_indx, 0x0409))
                    {
                        int strLen = (configString.len-2)/2;
                        if(strLen > 0) {
                            //Convert Unicode string to ASCII
                            char* tmp = new char[strLen + 1];
                            tmp[strLen] = '\0';
                            for(int i = 0; i < strLen; i++)
                                tmp[i] = configString.string[i];
                            
                            Log(Info, "     Config String -> %s", tmp);
                            delete tmp;
                        }
                    }
                }
            }
            else if (length == 9 && type == INTERFACE) // INTERFACE descriptor
            {
                struct INTERFACE_DESC* c = (struct INTERFACE_DESC*)startByte;
                Log(Info, "USBDevice Interface Desc: Num=%d Alt=%d NumEP=%d Class=%w Subclass=%w Protocol=%w StrIndx=%d", c->interface_num, c->alt_setting, c->num_endpoints, c->interface_class, c->interface_sub_class, c->interface_protocol, c->interface_indx);
                if(c->interface_num == 0) //First Interface
                {
                    if(this->classID == 0) this->classID = c->interface_class;
                    if(this->subclassID == 0) this->subclassID = c->interface_sub_class;
                    if(this->protocol == 0) this->protocol = c->interface_protocol;
                }
            }
            else if (length == 7 && type == ENDPOINT) // ENDPOINT descriptor
            {
                struct ENDPOINT_DESC* c = (struct ENDPOINT_DESC*)startByte;
                Log(Info, "USBDevice Endpoint Desc: Num=%d %s TransferType=%d MaxPacket=%d Interval=%d", c->end_point & 0xF, (c->end_point & (1<<7)) ? "In" : "Out", c->bm_attrbs & 0b11, c->max_packet_size, c->interval);

                this->endpoints.push_back(new USBEndpoint(c));
            }
            else if (type == HID) // HID descriptor
            {
                struct IF_HID_DESC* c = (struct IF_HID_DESC*)startByte;
                Log(Info, "USBDevice HID Desc: Release = %w CountryCode = %d NumDescriptors = %d", c->release, c->countryCode, c->numDescriptors);

                this->hidDescriptor = new uint8_t[c->len];
                MemoryOperations::memcpy(this->hidDescriptor, c, c->len);
            }
            else if (length == 0) // Unvallid entry
            {
                Log(Warning, "Found descriptor entry of size 0, assuming remaining data is invalid");
                break;
            }
            else
                Log(Warning, "Unknown part of ConfigDescriptor: length: %d type: %d", length, type);


            startByte += length;
        }
        delete configBuffer;
    }

    //Set Default Configuration
    if(!this->controller->SetConfiguration(this, 1)) {
        Log(Error, "Error Setting Device config to 1");
        return false;
    }

    //Print Class Info
    char* hexClass = Convert::IntToHexString((uint8_t)this->classID);
    for(int i = 0; i < numClassCodeStrings; i++)
        if(String::strncmp(hexClass, USBClassCodeStrings[i], 2) == true) {
            Log(Info, "USB Class: %s", USBClassCodeStrings[i] + 3);
            break;
        }

    ////////////
    // Driver Selection
    ////////////
    if(this->productID == 0xc52e && this->vendorID == 0x046d)
        this->driver = new USBComboReceiver(this);
    else if(this->classID == 0x08 && this->subclassID == 0x06 && this->protocol == 0x50) {
        this->driver = new USBMassStorageDriver(this);
    }
    else if(this->classID == 0x03 && this->protocol == 0x02) {
        this->driver = new USBMouse(this);
    }
    else if(this->classID == 0x03 && this->protocol == 0x01) {
        this->driver = new USBKeyboard(this);
    }

    ////////////
    // Initialize Driver
    ////////////
    if(this->driver != 0)
    {
        if(this->driver->Initialize())
            return true;
        else 
        {
            delete this->driver;
            this->driver = 0;
            return false;
        }
    }

    return false;
}
USBDevice::~USBDevice()
{
    for(USBEndpoint* endP : this->endpoints)
        delete endP;
}
void USBDevice::OnUnplugged()
{
    if(this->driver != 0) {
        Log(Info, "Unloading driver %s", this->driver->GetDriverName());
        this->driver->DeInitialize();
    }
    else
        Log(Info, "No driver found for this device");
}