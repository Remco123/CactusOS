#include <system/drivers/usb/usbmouse.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/log.h>
#include <system/system.h>
#include <system/usb/hidparser.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

USBMouse::USBMouse(USBDevice* dev)
: USBDriver(dev, "USB HID Mouse")
{ }

bool USBMouse::GetHIDProperty(struct HID_DATA* target, uint8_t* buffer, int bufLen, HID_USAGE item)
{
    MemoryOperations::memset(target, 0, sizeof(struct HID_DATA));
    target->type = ITEM_INPUT;
    target->path.node[0].u_page = HID_PAGE_USAGE::GEN_DESKTOP;
    target->path.node[0].usage = HID_USAGE::MOUSE;
    target->path.node[1].u_page = HID_PAGE_USAGE::GEN_DESKTOP;
    target->path.node[1].usage = HID_USAGE::POINTER;
    target->path.node[2].u_page = HID_PAGE_USAGE::GEN_DESKTOP;
    target->path.node[2].usage = item;
    //data.path.node[2].usage = USAGE_POINTER_Y;     // to get the Y Coordinate, comment X above and uncomment this line
    //data.path.node[2].usage = USAGE_POINTER_WHEEL; // to get the Wheel Coordinate, comment X above and uncomment this line
    target->path.size = 3;
    
    this->hidParser.report_desc = (const uint8_t*)buffer;
    this->hidParser.report_desc_size = bufLen;

    return this->hidParser.FindObject(target);
}

bool USBMouse::Initialize()
{
    if(this->device->hidDescriptor == 0)
        return false;

    // Find Interrupt Endpoint
    for(ENDPOINT_DESC* ep : this->device->endpoints) {
        if((ep->bm_attrbs & 0b11) == 0b11) { // Interrupt Endpoint
            if((ep->end_point & (1<<7)) == (1<<7)) // In
                this->InInterruptEndpoint = ep->end_point & 0b1111;
        }
    }

    // Check if endpoint is found
    if(this->InInterruptEndpoint == -1)
        return false;

    // Send set-idle request to device
    if(!this->device->controller->ControlOut(this->device, 0, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, REQUEST_SET_IDLE));
        Log(Warning, "USBMouse not reacting to SetIdle request");
    
    // Get length of HID report from Interface HID Descriptor
    uint16_t reportDescriptorLength = *(uint16_t*)(this->device->hidDescriptor + sizeof(IF_HID_DESC) + 1);
    if(reportDescriptorLength == 0)
        return false;

    // Receive Report Descriptor
    uint8_t* reportDescriptorBuffer = new uint8_t[reportDescriptorLength];
    if(!this->device->controller->ControlIn(this->device, reportDescriptorBuffer, reportDescriptorLength, DEV_TO_HOST | REQ_TYPE_STNDRD | RECPT_INTERFACE, GET_DESCRIPTOR, 0x22))
        return false;
    
    // Extract data returned from descriptor
    bool b1 = GetHIDProperty(&this->hidX, reportDescriptorBuffer, reportDescriptorLength, HID_USAGE::POINTER_X);
    bool b2 = GetHIDProperty(&this->hidY, reportDescriptorBuffer, reportDescriptorLength, HID_USAGE::POINTER_Y);
    bool b3 = GetHIDProperty(&this->hidZ, reportDescriptorBuffer, reportDescriptorLength, HID_USAGE::POINTER_WHEEL);

    // Since all required data is present from the discriptor we can use the custom protocol
    if(b1 && b2 && b3)
        this->useCustomReport = true;

    // Send SET_PROTOCOL request to device
    if(!this->device->controller->ControlOut(this->device, 0, 0, HOST_TO_DEV | REQ_TYPE_CLASS | RECPT_INTERFACE, REQUEST_SET_PROTOCOL, 0, this->useCustomReport ? 1 : 0))
        return false;

    uint8_t buf[4];
    bool suc = this->device->controller->InterruptIn(this->device, buf, 4, this->InInterruptEndpoint);

    return true;
}

void USBMouse::DeInitialize()
{

}