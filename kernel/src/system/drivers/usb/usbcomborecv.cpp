#include <system/drivers/usb/usbcomborecv.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/log.h>
#include <system/system.h>
#include <system/usb/hidparser.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBComboReceiver::USBComboReceiver(USBDevice* dev)
: USBDriver(dev, "USB Combo Receiver"), Keyboard(KeyboardType::USB)
{
    MemoryOperations::memset(this->prevPacket, 0, sizeof(this->prevPacket));
}

bool USBComboReceiver::Initialize()
{
    if(this->device->hidDescriptor == 0) // If no HID report is found we assume the device is not supported
        return false;

    // Since this is a device with multiple interfaces we need to request the config descriptor again
    uint8_t* configBuffer = this->device->controller->GetConfigDescriptor(this->device);
    if(configBuffer) {
        uint8_t* startByte = configBuffer;
        uint8_t* endByte = configBuffer + *(uint16_t*)(configBuffer + 2);

        // Should we set the found endpoints to the mouse interface or the keyboard interface?
        bool endpToMouse = false;

        // Loop through all data
        while ((uint32_t)startByte < (uint32_t)endByte) {
            uint8_t length = startByte[0];
            uint8_t type = startByte[1];

            if (length == 9 && type == INTERFACE) {
                struct INTERFACE_DESC* c = (struct INTERFACE_DESC*)startByte;
                if(c->interface_protocol == 0x02) {
                    endpToMouse = true;
                    this->mouseInterface = c->interface_num;
                }
                else if(c->interface_protocol == 0x01) {
                    endpToMouse = false;
                    this->keyboardInterface = c->interface_num;
                }
            }
            
            else if (length == 7 && type == ENDPOINT) // ENDPOINT descriptor
            {
                USBEndpoint ep = USBEndpoint((struct ENDPOINT_DESC*)startByte);

                if(ep.type == EndpointType::Interrupt) { // Interrupt Endpoint
                    if(ep.dir == EndpointDirection::In) { // In
                        if(endpToMouse)
                            this->mouseIntEndpoint = ep.endpointNumber;
                        else
                            this->keyboardIntEndpoint = ep.endpointNumber;
                    }
                }
                
            }
            else if (length == 0) // Unvalid entry
                break;
            else
                Log(Warning, "Unknown part of ConfigDescriptor: length: %d type: %d", length, type);


            startByte += length;
        }
        delete configBuffer;
    }

    // Check if both endpoints are found
    if(this->mouseIntEndpoint == -1 || this->keyboardIntEndpoint == -1)
        return false;

    // Check if both interfaces are found
    if(this->mouseInterface == -1 || this->keyboardInterface == -1)
        return false;

    Log(Info, "USBCombo Receiver initializing.... Keyboard -> %d,%d  Mouse -> %d,%d", this->keyboardIntEndpoint, this->keyboardInterface, this->mouseIntEndpoint, this->mouseInterface);

    /////////////////
    // Keyboard Initialization
    /////////////////
    
    // Send set-idle request to device
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_IDLE, 0, 0, this->keyboardInterface))
        Log(Warning, "USBCombo Keyboard not reacting to SetIdle request");

    // Send set-protocol request to device for the boot protocol
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_PROTOCOL, 0, 0, this->keyboardInterface))
        return false;

    // Start recieving interrupt packets from device
    this->device->controller->InterruptIn(this->device, 8, this->keyboardIntEndpoint);

    /////////////////
    // Mouse Initialization
    // TODO: Don't use boot protocol for this device
    /////////////////
    
    // Send set-idle request to device
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_IDLE, 0, 0, this->mouseInterface))
        Log(Warning, "USBCombo Mouse not reacting to SetIdle request");

    // Send set-protocol request to device for the boot protocol
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_PROTOCOL, 0, 0, this->mouseInterface))
        return false;

    // Start recieving interrupt packets from device
    this->device->controller->InterruptIn(this->device, 4, this->mouseInterface);

    return true;
}
void USBComboReceiver::DeInitialize()
{ }
bool USBComboReceiver::HandleInterruptPacket(InterruptTransfer_t* transfer)
{
    Log(Info, "USB Combo received packet -> %d", transfer->endpoint);

    if(transfer->endpoint == this->mouseIntEndpoint)
    {

    }
    else if(transfer->endpoint == this->keyboardIntEndpoint)
    {

    }
    return true;
}