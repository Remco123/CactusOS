#include <system/drivers/usb/usbkeyboard.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/log.h>
#include <system/system.h>
#include <system/usb/hidparser.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBKeyboard::USBKeyboard(USBDevice* dev)
: USBDriver(dev, "USB HID Keyboard")
{ }

bool USBKeyboard::Initialize()
{
    if(this->device->hidDescriptor == 0)
        return false;

    // Find Interrupt Endpoint
    for(USBEndpoint* ep : this->device->endpoints) {
        if(ep->type == EndpointType::Interrupt) { // Interrupt Endpoint
            if(ep->dir == EndpointDirection::In) { // In
                this->InInterruptEndpoint = ep->endpointNumber;
                break;
            }
        }
    }

    // Check if endpoint is found
    if(this->InInterruptEndpoint == -1)
        return false;

    // Send set-idle request to device
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_IDLE))
        Log(Warning, "USBKeyboard not reacting to SetIdle request");

    // Send SET_PROTOCOL request to device for the boot protocol
    if(!this->device->controller->ControlOut(this->device, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, HID_REQUEST_SET_PROTOCOL, 0, 0))
        return false;

    // Start recieving interrupt packets from device
    this->device->controller->InterruptIn(this->device, 8, this->InInterruptEndpoint);

    return true;
}

void USBKeyboard::DeInitialize()
{ } // Keyboard does not have any requirements for unplugging

bool USBKeyboard::HandleInterruptPacket(InterruptTransfer_t* transfer)
{
    uint8_t* packet = transfer->bufferPointer;
    Log(Info, "Received keyboard packet! %d %d %d %d %d %d %d %d", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);
    
    

    return true; // Rescedule
}