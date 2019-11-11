#include <system/drivers/usb/controllers/uhci.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

UHCIController::UHCIController(PCIDevice* device)
: USBController(UHCI), Driver("UHCI USB Controller", "Controller for a UHCI device"), InterruptHandler(IDT_INTERRUPT_OFFSET + device->interrupt)
{
    this->pciDevice = device;
}

bool UHCIController::Initialize()
{
    // We do not want memory mapped controllers
    if(System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 4).type == BaseAddressRegisterType::MemoryMapping)
        return false;
    
    uint32_t base = pciDevice->portBase;

    for(int i = 0; i < 5; i++)
    {
        outportw(base + UHCI_COMMAND, 0x0004);
        System::pit->Sleep(11);
        outportw(base + UHCI_COMMAND, 0x0000);
    }

    //Check if command register has default value
    if(inportw(base+UHCI_COMMAND) != 0x0000) return false;
    //Check if status register has default value
    if(inportw(base+UHCI_STATUS) != 0x0020) return false;
    //Clear status register
    outportw(base+UHCI_STATUS, 0x00FF);

    //Is the SOF register its default value?
    if(inportw(base+UHCI_SOF_MOD) != 0x40) return false;

    //Set bit 1 in command register, should be reset automaticly
    outportw(base+UHCI_COMMAND, 0x0002);

    //Give the controller some time
    System::pit->Sleep(42);

    //Bit should be clear
    if(inportw(base+UHCI_COMMAND) & 0x0002) return false;

    //Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void UHCIController::Setup()
{
    //Enable 3 interrupts
    outportw(pciDevice->portBase + UHCI_INTERRUPT, 0b1011);
    //Set frame number register to 0
    outportw(pciDevice->portBase + UHCI_FRAME_NUM, 0x0000);

    // Disable Legacy Support
    if(System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, UHCI_LEGACY) != 0)
        System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, UHCI_LEGACY, 0x8F00);

    //Allocate Stack frame
    this->frameList = (uint32_t*)KernelHeap::allignedMalloc(1024 * sizeof(uint32_t) * 2, 4096, &this->frameListPhys);
    MemoryOperations::memset(this->frameList, 0x0, 1024 * sizeof(uint32_t) * 2);

    //And set all entries to 0
    for(int i = 0; i < 1024; i++)
        this->frameList[i] = 0x00000001;

    //Set stack frame address
    outportl(pciDevice->portBase + UHCI_FRAME_BASE, frameListPhys);
    //Set start of frame register to 64
    outportb(pciDevice->portBase + UHCI_SOF_MOD, 0x40);
    //Clear status register
    outportw(pciDevice->portBase + UHCI_STATUS, 0xFFFF);
    //Finally enable controller (Max Packet bit, Configured bit and run bit)
    outportw(pciDevice->portBase + UHCI_COMMAND, (1<<7) | (1<<6) | (1<<0));

    int dev_address = 1;
    struct DEVICE_DESC dev_desc;

    /////////////
    // Device Enumeration
    /////////////
    uint8_t port = 0x10; //Start of ports
    while (PortPresent(port)) {
        // reset the port
        if (ResetPort(port)) {
            // is a device is attached?
            if (inportw(pciDevice->portBase + port) & 1) {
                bool ls_device = (inportw(pciDevice->portBase + port) & (1<<8)) ? true : false;
                Log(Info, "UHCI, Found Device at port %d, low speed = %b", (port - 0x10) >> 1, ls_device);

                // get first 8 bytes of descriptor
                if (GetDescriptor(&dev_desc, ls_device, 0, 8, 8, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE)) {
                    // reset the port again
                    ResetPort(port);
                    // set address of device
                    if (SetAddress(dev_address, ls_device)) {
                        //Setup device
                        USBDevice* newDev = new USBDevice();
                        newDev->controller = this;
                        newDev->devAddress = dev_address;
                        newDev->portNum = (port - 0x10) / 2;
                        newDev->uhciProperties.lowSpeedDevice = ls_device;
                        newDev->uhciProperties.maxPacketSize = dev_desc.max_packet_size;
                        System::usbManager->AddDevice(newDev);
                    } else
                        Log(Error, "Error setting device address.");
                } else
                    Log(Error, "Error getting first 8 bytes of descriptor.");
            }
        }
        port += 2;  // move to next port
    }
}

bool UHCIController::PortPresent(uint8_t port)
{
    uint32_t base = pciDevice->portBase;

    // if bit 7 is 0, not a port
    if ((inportw(base+port) & 0x0080) == 0) return false;

    // try to clear it
    outportw(base+port, inportw(base+port) & ~0x0080);
    if ((inportw(base+port) & 0x0080) == 0) return false;

    // try to write/clear it
    outportw(base+port, inportw(base+port) | 0x0080);
    if ((inportw(base+port) & 0x0080) == 0) return false;

    // let's see if we write a 1 to bits 3:1, if they come back as zero
    outportw(base+port, inportw(base+port) | 0x000A);
    if ((inportw(base+port) & 0x000A) != 0) return false;
  
    // we should be able to assume this is a valid port if we get here
    return true;
}
bool UHCIController::ResetPort(uint8_t port)
{
    uint32_t base = pciDevice->portBase;
    bool ret = false;
    
    outportw(base+port, inportw(base + port) | (1<<9));
    System::pit->Sleep(USB_TDRSTR);
    outportw(base + port, inportw(base+port) & ~(1<<9));
    
    for (int i = 0; i < 10; i++) {
        System::pit->Sleep(USB_TRSTRCY);  // hold for USB_TRSTRCY ms (reset recovery time)
        
        uint16_t val = inportw(base + port);
        
        // if bit 0 is clear, nothing attached, don't enable
        if (!(val & (1<<0))) {
            ret = true;
            break;
        }
        
        // if either enable_change or connection_change, clear them and continue.
        if (val & ((1<<3) | (1<<1))) {
            outportw(base + port, val & UHCI_PORT_WRITE_MASK);
            continue;
        }
        
        // if the enable bit is set, break.
        if (val & (1<<2)) {
            ret = true;
            break;
        }
        
        // else, set the enable bit
        outportw(base + port, val | (1<<2));
    }
    
    return ret;
}

// set up a queue, and enough TD's to get 'size' bytes
bool UHCIController::GetDescriptor(void* devDesc, const bool lsDevice, const int devAddress, const int packetSize, const int size, const uint8_t requestType, const uint8_t request, const uint16_t valueLow, const uint16_t valueHigh, const uint16_t index) {
    //Create Request Packet
    REQUEST_PACKET requestPacket;
    {
        requestPacket.request_type = requestType;
        requestPacket.request = request;
        requestPacket.value = (valueHigh >> 8) | (valueLow << 8);
        requestPacket.index = index;
        requestPacket.length = size;
    }

    //Create temporary buffer and clear it
    uint8_t* tempBuf = new uint8_t[size];
    MemoryOperations::memset(tempBuf, 0, size);

    int i = 1, t, sz = size;
    uint32_t frameListVirt = (uint32_t)frameList;
    
    struct UHCI_QUEUE_HEAD queue;
    struct UHCI_TRANSFER_DESCRIPTOR td[10];
    
    queue.horz_ptr = 0x00000001;
    queue.vert_ptr = (frameListPhys + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD));  // 128 to skip past buffers
    
    td[0].link_ptr = ((queue.vert_ptr & ~0xF) + sizeof(struct UHCI_TRANSFER_DESCRIPTOR));
    td[0].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
    td[0].info = (7<<21) | ((devAddress & 0x7F)<<8) | TOKEN_SETUP;
    td[0].buff_ptr = frameListPhys + 4096;
    
    while ((sz > 0) && (i<9)) {
        td[i].link_ptr = ((td[i-1].link_ptr & ~0xF) + sizeof(struct UHCI_TRANSFER_DESCRIPTOR));
        td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
        t = ((sz <= packetSize) ? sz : packetSize);
        td[i].info = ((t-1)<<21) | ((i & 1) ? (1<<19) : 0) | ((devAddress & 0x7F)<<8) | TOKEN_IN;
        td[i].buff_ptr = frameListPhys + 4096 + (8 * i);
        sz -= t;
        i++;
    }
    
    td[i].link_ptr = 0x00000001;
    td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (1<<24) | (0x80 << 16);
    td[i].info = (0x7FF<<21) | (1<<19) | ((devAddress & 0x7F)<<8) | TOKEN_OUT;
    td[i].buff_ptr = 0x00000000;
    i++; // for a total count
    
    // make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);
    
    // now move our queue into the physical memory allocated
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 0), &requestPacket, 8);
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 8), tempBuf, size);
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 128), &queue, sizeof(struct UHCI_QUEUE_HEAD));
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD)), &td[0], sizeof(struct UHCI_TRANSFER_DESCRIPTOR) * 10);
    
    // mark the first stack frame pointer
    frameList[0] = (frameListPhys + 4096 + 128) | QUEUE_HEAD_Q;
    
    // wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        frameList[0] = 1; // mark the first stack frame pointer invalid
        return false;
    }

    Log(Info, "Frame: %d - USB transaction completed", inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111);
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    frameList[0] = 1; // mark the first stack frame pointer invalid
    
    // copy the stack frame back to our local buffer
    MemoryOperations::memcpy(tempBuf, (void*)(frameListVirt + 4096 + 8), size);
    MemoryOperations::memcpy(&queue, (void*)(frameListVirt + 4096 + 128), sizeof(struct UHCI_QUEUE_HEAD));
    MemoryOperations::memcpy(&td[0], (void*)(frameListVirt + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD)), (sizeof(struct UHCI_TRANSFER_DESCRIPTOR) * 10));
    
    // check the TD's for error
    for (t=0; t<i; t++) {
        if (((td[t].reply & (0xFF<<16)) != 0)) {
            delete tempBuf;
            return false;
        }
    }
    
    // copy the descriptor to the passed memory block
    MemoryOperations::memcpy(devDesc, tempBuf, size);
    
    delete tempBuf;
    return true;
}

bool UHCIController::SetAddress(const int dev_address, const bool ls_device) {
    // our setup packet (with the third byte replaced below)
    static uint8_t setup_packet[8] = { 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    int i, timeout;
    uint32_t io_base = pciDevice->portBase;
    uint32_t frameListVirt = (uint32_t)frameList;
    
    // one queue and two TD's
    struct UHCI_QUEUE_HEAD queue;
    struct UHCI_TRANSFER_DESCRIPTOR td[2];
    
    setup_packet[2] = (uint8_t)dev_address;
    
    queue.horz_ptr = 0x00000001;
    queue.vert_ptr = (frameListPhys + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD));  // 128 to skip past buffers
    
    td[0].link_ptr = ((queue.vert_ptr & ~0xF) + sizeof(struct UHCI_TRANSFER_DESCRIPTOR));
    td[0].reply = (ls_device ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
    td[0].info = (7<<21) | (0<<8) | TOKEN_SETUP;
    td[0].buff_ptr = frameListPhys + 4096;
    
    td[1].link_ptr = 0x00000001;
    td[1].reply = (ls_device ? (1<<26) : 0) | (3<<27) | (1<<24) | (0x80 << 16);
    td[1].info = (0x7FF<<21) | (1<<19) | (0<<8) | TOKEN_IN;
    td[1].buff_ptr = 0x00000000;
    
    // make sure status:int bit is clear
    outportw(io_base+UHCI_STATUS, 1);
    
    // now move our queue into the physicall memory allocated
    MemoryOperations::memcpy((void*)(frameListVirt + 4096), setup_packet, 8);
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 128), &queue, sizeof(struct UHCI_QUEUE_HEAD));
    MemoryOperations::memcpy((void*)(frameListVirt + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD)), &td[0], (sizeof(struct UHCI_TRANSFER_DESCRIPTOR) * 2));
   
    // mark the first stack frame pointer
    frameList[0] = (frameListPhys + 4096 + 128) | QUEUE_HEAD_Q;
    
    // wait for the IOC to happen
    timeout = 10000; // 10 seconds
    while (!(inportw(io_base+UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        frameList[0] = 1; // mark the first stack frame pointer invalid
        return false;
    }
    outportw(io_base+UHCI_STATUS, 1);  // acknowledge the interrupt
    
    frameList[0] = 1; // mark the first stack frame pointer invalid
    
    // copy the stack frame back to our local buffer
    MemoryOperations::memcpy(&queue, (void*)(frameListVirt + 4096 + 128), sizeof(struct UHCI_QUEUE_HEAD));
    MemoryOperations::memcpy(&td[0], (void*)(frameListVirt + 4096 + 128 + sizeof(struct UHCI_QUEUE_HEAD)), (sizeof(struct UHCI_TRANSFER_DESCRIPTOR) * 2));
   
    // check the TD's for error
    for (i=0; i<2; i++) {
        if ((td[i].reply & (0xFF<<16)) != 0)
            return false;
    }

    return true;
}
uint32_t UHCIController::HandleInterrupt(uint32_t esp)
{
    Log(Info, "UHCI Interrupt");
    uint16_t val = inportw(pciDevice->portBase + UHCI_STATUS);

    if(val == 0) // Interrupt came from another UHCI device
    {
        Log(Info, "Interrupt came from another UHCI device!");
        return esp;
    }

    if (val & UHCI_STS_RESUME_DETECT)
    {
        Log(Info, "Resume Detect");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_RESUME_DETECT); // reset interrupt
    }

    if (val & UHCI_STS_HCHALTED)
    {
        Log(Error, "Host Controller Halted");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HCHALTED); // reset interrupt
    }

    if (val & UHCI_STS_HC_PROCESS_ERROR)
    {
        Log(Error, "Host Controller Process Error");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HC_PROCESS_ERROR); // reset interrupt
    }

    if (val & UHCI_STS_USB_ERROR)
    {
        Log(Error, "USB Error");
        int num = inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111;
        Log(Info, "Frame Base: %x Frame Num: %d Frame: %x", inportl(pciDevice->portBase + UHCI_FRAME_BASE), num, this->frameList[num]);
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_USB_ERROR); // reset interrupt
    }

    if (val & UHCI_STS_HOST_SYSTEM_ERROR)
    {
        Log(Error, "Host System Error");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HOST_SYSTEM_ERROR); // reset interrupt
    }

    return esp;
}

/////////
// USB Controller Functions
/////////
bool UHCIController::GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device)
{
    return GetDescriptor(dev_desc, device->uhciProperties.lowSpeedDevice, device->devAddress, device->uhciProperties.maxPacketSize, sizeof(struct DEVICE_DESC), STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE);
}
bool UHCIController::GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang)
{
    if(!GetDescriptor(stringDesc, device->uhciProperties.lowSpeedDevice, device->devAddress, device->uhciProperties.maxPacketSize, 8, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, lang, index))
        return false;
        
    int totalSize = stringDesc->len;
    return GetDescriptor(stringDesc, device->uhciProperties.lowSpeedDevice, device->devAddress, device->uhciProperties.maxPacketSize, totalSize, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, lang, index);
}