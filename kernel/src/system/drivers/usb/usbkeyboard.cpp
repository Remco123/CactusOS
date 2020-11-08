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
: USBDriver(dev, "USB HID Keyboard"), Keyboard(KeyboardType::USB)
{ 
    MemoryOperations::memset(this->prevPacket, 0, sizeof(this->prevPacket));
}

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

    // Add ourself to the list of known keyboards
    System::keyboardManager->keyboards.push_back(this);

    return true;
}

void USBKeyboard::DeInitialize()
{ 
    // Remove ourself from the list of known keyboards
    System::keyboardManager->keyboards.Remove(this);
}

bool USBKeyboard::HandleInterruptPacket(InterruptTransfer_t* transfer)
{    
    uint8_t* packet = transfer->bufferPointer;

    // Update internal modifier keys
    uint8_t modifier = packet[0];
    this->status.LeftControl = modifier & (1<<0);
    this->status.LeftShift = modifier & (1<<1);
    this->status.Alt = modifier & (1<<2);
    this->status.RightControl = modifier & (1<<4);
    this->status.RightShift = modifier & (1<<5);
    this->status.AltGr = modifier & (1<<6);

    // Check for each keypacket field if there are any updates
    for(int i = 2; i < 8; i++) {
        uint8_t key = packet[i];
        int pos[2];

        bool b1 = this->ContainsKey(key, packet, &pos[0]);
        bool b2 = this->ContainsKey(key, this->prevPacket, &pos[1]);

        if(b1 && b2) // Key is still pressed
            continue;
        
        if(b1 && !b2) // Key pressed for first time
            System::keyboardManager->HandleKeyChange(this, packet[pos[0]], true);
        else if(!b1 && b2) // Key is released
            System::keyboardManager->HandleKeyChange(this, this->prevPacket[pos[1]], false);
    }

    MemoryOperations::memcpy(this->prevPacket, packet, sizeof(this->prevPacket));
    return true; // Rescedule
}

void USBKeyboard::UpdateLEDS()
{
	uint8_t code = 0;

	if(System::keyboardManager->sharedStatus.NumLock)
		code |= 1 << 0;
		
	if(System::keyboardManager->sharedStatus.CapsLock)
		code |= 1 << 1;

    if(!this->device->controller->ControlOut(this->device, 1, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, 0x9, 0x02, 0, 0))
        Log(Warning, "USB KBD Could not update Leds");
}