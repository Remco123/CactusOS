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
    this->numRootPorts = 0;
}

void UHCIController::InsertQueue(uhci_queue_head_t* queue, uint32_t queuePhys, const int queueIndex)
{
    uhci_queue_head_t* curQueue = &this->queueStackList[queueIndex];

    if((curQueue->vert_ptr & QUEUE_HEAD_T) == QUEUE_HEAD_T) { // First time a queue is added to this chain
        curQueue->vert_ptr = queuePhys | QUEUE_HEAD_Q;
        curQueue->nextQueuePointer = queue;

        queue->horz_ptr = QUEUE_HEAD_T; // Should be set already, just to be sure
        queue->parentQueuePointer = curQueue;
    }
    else // Queue can be added horizontaly to existing entries
    {
        while(curQueue->nextQueuePointer != 0)
            curQueue = (uhci_queue_head_t*)curQueue->nextQueuePointer;

        queue->horz_ptr = curQueue->horz_ptr;
        curQueue->horz_ptr = queuePhys | QUEUE_HEAD_Q;

        queue->nextQueuePointer = curQueue->nextQueuePointer;

        curQueue->nextQueuePointer = queue;
        queue->parentQueuePointer = curQueue;
    }
}

void UHCIController::RemoveQueue(uhci_queue_head_t* queue, const int queueIndex)
{
    uhci_queue_head_t* parent = (uhci_queue_head_t*)queue->parentQueuePointer;
    uhci_queue_head_t* child = (uhci_queue_head_t*)queue->nextQueuePointer;

    if(parent == &this->queueStackList[queueIndex]) { // Queue is Below first Queue head of skeleton 
        parent->vert_ptr = queue->horz_ptr;
        if(child)
            child->parentQueuePointer = parent;
    }
    else { // Queue is not the first entry in the chain
        parent->horz_ptr = queue->horz_ptr;
        parent->nextQueuePointer = child;

        if(child)
            child->parentQueuePointer = parent;
    }
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
    this->frameList = (uint32_t*)KernelHeap::allignedMalloc(1024 * sizeof(uint32_t), 4096, &this->frameListPhys);
    MemoryOperations::memset(this->frameList, 0x0, 1024 * sizeof(uint32_t));

    //Allocate queue stack list
    uint32_t queuePhysStart = 0;
    this->queueStackList = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t) * NUM_UHCI_QUEUES, 16, &queuePhysStart);
    MemoryOperations::memset(this->queueStackList, 0x0, sizeof(uhci_queue_head_t) * NUM_UHCI_QUEUES);

    //Set all queue entries to invalid
    for(int i = 0; i < NUM_UHCI_QUEUES; i++) {
        this->queueStackList[i].horz_ptr = QUEUE_HEAD_T;
        this->queueStackList[i].vert_ptr = QUEUE_HEAD_T;
    }

    //Setup queue entries to point to each other
    //128 -> 64 -> 32 -> 16 -> 8 -> 4 -> 2 -> 1 -> QControl -> QBulk
    for(int i = 0; i < NUM_UHCI_QUEUES - 1; i++) {
        this->queueStackList[i].horz_ptr = (queuePhysStart + (i + 1) * sizeof(uhci_queue_head_t)) | QUEUE_HEAD_Q;
    }

    //Setup UHCI stack frame
    for(int i = 0; i < 1024; i++) {
        int queueStart = QUEUE_Q1;
        if((i + 1) % 2 == 0) queueStart--;
        if((i + 1) % 4 == 0) queueStart--;
        if((i + 1) % 8 == 0) queueStart--;
        if((i + 1) % 16 == 0) queueStart--;
        if((i + 1) % 32 == 0) queueStart--;
        if((i + 1) % 64 == 0) queueStart--;
        if((i + 1) % 128 == 0) queueStart--;

        this->frameList[i] = (queuePhysStart + queueStart * sizeof(uhci_queue_head_t)) | QUEUE_HEAD_Q;
    }

    //Set stack frame address
    outportl(pciDevice->portBase + UHCI_FRAME_BASE, frameListPhys);
    //Set start of frame register to 64
    outportb(pciDevice->portBase + UHCI_SOF_MOD, 0x40);
    //Clear status register
    outportw(pciDevice->portBase + UHCI_STATUS, 0xFFFF);
    //Finally enable controller (Max Packet bit, Configured bit and run bit)
    outportw(pciDevice->portBase + UHCI_COMMAND, (1<<7) | (1<<6) | (1<<0));

    /////////////
    // Device Enumeration
    /////////////
    uint8_t port = 0x10; //Start of ports
    while (PortPresent(port)) {
        numRootPorts++;
        // reset the port
        if (ResetPort(port)) {
            // is a device is attached?
            if (inportw(pciDevice->portBase + port) & 1) {
                SetupNewDevice(port);
            }
        }
        port += 2;  // move to next port
    }
}
void UHCIController::SetupNewDevice(uint8_t port)
{
    static int dev_address = 1;
    struct DEVICE_DESC dev_desc;

    bool ls_device = (inportw(pciDevice->portBase + port) & (1<<8)) ? true : false;
    Log(Info, "UHCI, Found Device at port %d, low speed = %b", (port - 0x10) / 2, ls_device);

    // get first 8 bytes of descriptor
    if (ControlIn(&dev_desc, ls_device, 0, 8, 8, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE)) 
    {
        // reset the port again
        ResetPort(port);
        // set address of device
        if (ControlOut(ls_device, 0, dev_desc.max_packet_size, 0, STDRD_SET_REQUEST, SET_ADDRESS, 0, dev_address)) 
        {
            //Setup device
            USBDevice* newDev = new USBDevice();
            newDev->controller = this;
            newDev->devAddress = dev_address++;
            newDev->portNum = (port - 0x10) / 2;
            newDev->uhciProperties.lowSpeedDevice = ls_device;
            newDev->uhciProperties.maxPacketSize = dev_desc.max_packet_size;
            System::usbManager->AddDevice(newDev);
        } 
        else
            Log(Error, "Error setting device address.");
    } 
    else
        Log(Error, "Error getting first 8 bytes of descriptor.");
}
void UHCIController::ControllerChecksThread()
{
    for(int i = 0; i < numRootPorts; i++)
    {
        uint8_t port = 0x10 + i*2;

        uint16_t portSts = inportw(pciDevice->portBase + port);
        if(portSts & (1<<1)) //Port Connection Change Bit
        {
            outportw(pciDevice->portBase + port, (1<<1));
            Log(Info, "UHCI Port %d Connection change, now %s", i, (portSts & (1<<0)) ? "Connected" : "Not Connected");

            if((portSts & (1<<0)) == 1) { //Connected
                if(ResetPort(port))
                    SetupNewDevice(port);
            }
            else { //Not Connected
                System::usbManager->RemoveDevice(this, i);
            }
        }
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
            ret = false;
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

int UHCIController::CheckTransferDone(u_transferDescriptor_t* td, int numTDs)
{
    bool noError = true;

    // Check if any TD has a error or a NAK is received. If not then the transfer is complete.
    for (int i = 0; i < numTDs; i++) {
        uint8_t status = td[i].reply & (0xFF << 16);
        if (status != 0) { // Check if any error bit is set or not done
            if(status & (1<<3) == (1<<3))
                return 2; // (Only) NAK bit set
            
            noError = false;
            break;
        }
    }

    return noError ? 0 : 1;
}

// set up a queue, and enough TD's to get 'size' bytes
bool UHCIController::ControlIn(void* targ, const bool lsDevice, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) {
    //Create Request Packet
    uint32_t requestPacketPhys;
    REQUEST_PACKET* requestPacket = (REQUEST_PACKET*)KernelHeap::allignedMalloc(sizeof(REQUEST_PACKET), 16, &requestPacketPhys);
    {
        requestPacket->request_type = requestType;
        requestPacket->request = request;
        requestPacket->value = (valueHigh << 8) | valueLow;
        requestPacket->index = index;
        requestPacket->length = len;
    }

    //Create temporary buffer and clear it
    uint32_t returnBufPhys;
    uint8_t* returnBuf = (uint8_t*)KernelHeap::malloc(len, &returnBufPhys);
    MemoryOperations::memset(returnBuf, 0, len);
    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    u_transferDescriptor_t* td = (u_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(u_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(u_transferDescriptor_t) * 10);

    //Allocate queue head
    uint32_t queuePhys;
    uhci_queue_head_t* queue = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t), 16, &queuePhys);
    MemoryOperations::memset(queue, 0, sizeof(uhci_queue_head_t));
    queue->horz_ptr = QUEUE_HEAD_T;
    queue->vert_ptr = tdPhys;

    //Setup Transfer Descriptor
    td[0].link_ptr = ((tdPhys & ~0xF) + sizeof(u_transferDescriptor_t));
    td[0].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
    td[0].info = (7<<21) | ((devAddress & 0x7F)<<8) | (ENDP_CONTROL<<15) | TOKEN_SETUP;
    td[0].buff_ptr = requestPacketPhys;
    
    int i = 1;
    int sz = len;
    //Transfer Descriptors depending on size of request
    while ((sz > 0) && (i<9)) {
        td[i].link_ptr = ((td[i-1].link_ptr & ~0xF) + sizeof(u_transferDescriptor_t));
        td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
        int t = ((sz <= packetSize) ? sz : packetSize);
        td[i].info = ((t-1)<<21) | ((i & 1) ? (1<<19) : 0)  | (ENDP_CONTROL<<15) | ((devAddress & 0x7F)<<8) | TOKEN_IN;
        td[i].buff_ptr = returnBufPhys + (8 * (i-1));
        sz -= t;
        i++;
    }
    

    //Acknowledge Transfer Descriptor (Status)
    td[i].link_ptr = 0x00000001;
    td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (1<<24) | (0x80 << 16);
    td[i].info = (0x7FF<<21) | (1<<19) | (ENDP_CONTROL<<15) | ((devAddress & 0x7F)<<8) | TOKEN_OUT;
    td[i].buff_ptr = 0x00000000;
    i++; // for a total count
    
    // make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);

    //Instert queue into QControl list
    InsertQueue(queue, queuePhys, QUEUE_QControl);
    
    // wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        RemoveQueue(queue, QUEUE_QControl);
        return false;
    }

    //Log(Info, "Frame: %d - USB transaction completed", inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111);
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    RemoveQueue(queue, QUEUE_QControl);
    
    bool ret = true;
    // check the TD's for error
    for (int t = 0; t < i; t++) {
        if (((td[t].reply & (0xFF << 16)) != 0)) { //Check if any error bit is set
            ret = false;
            break;
        }
    }
    
    if(ret) {
        // copy the descriptor to the passed memory block
        MemoryOperations::memcpy(targ, returnBuf, len);
    }
    
    //Free temporary buffer
    KernelHeap::free(returnBuf);
    //Free td's
    KernelHeap::allignedFree(td);
    //Free queue head
    KernelHeap::allignedFree(queue);
    //Free Packet
    KernelHeap::allignedFree(requestPacket);

    return ret;
}

bool UHCIController::ControlOut(const bool lsDevice, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) {
    //Create setupPacket
    uint32_t setupPacketPhys;
    REQUEST_PACKET* setupPacket = (REQUEST_PACKET*)KernelHeap::allignedMalloc(sizeof(REQUEST_PACKET), 16, &setupPacketPhys);
    {
        setupPacket->request_type = requestType;
        setupPacket->request = request;
        setupPacket->value = (valueHigh << 8) | valueLow;
        setupPacket->index = index;
        setupPacket->length = len;
    }
    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    u_transferDescriptor_t* td = (u_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(u_transferDescriptor_t) * 2, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(u_transferDescriptor_t) * 2);

    //Allocate queue head
    uint32_t queuePhys;
    uhci_queue_head_t* queue = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t), 16, &queuePhys);
    MemoryOperations::memset(queue, 0, sizeof(uhci_queue_head_t));
    queue->horz_ptr = QUEUE_HEAD_T;
    queue->vert_ptr = tdPhys;
    
    //Create set address td
    td[0].link_ptr = ((tdPhys & ~0xF) + sizeof(u_transferDescriptor_t));
    td[0].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
    td[0].info = (7<<21) | (ENDP_CONTROL<<15) | ((devAddress & 0x7F)<<8) | TOKEN_SETUP;
    td[0].buff_ptr = setupPacketPhys;
    
    //Create status td
    td[1].link_ptr = 0x00000001;
    td[1].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (1<<24) | (0x80 << 16);
    td[1].info = (0x7FF<<21) | (1<<19) | (ENDP_CONTROL<<15) | ((devAddress & 0x7F)<<8) | TOKEN_IN;
    td[1].buff_ptr = 0x00000000;
    
    //Make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);
   
    //Instert queue into QControl list
    InsertQueue(queue, queuePhys, QUEUE_QControl);
    
    //Wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        RemoveQueue(queue, QUEUE_QControl);
        return false;
    }
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    RemoveQueue(queue, QUEUE_QControl);
   
    //Check the TD's for error
    bool ret = true;
    for (int i = 0; i < 2; i++) {
        if ((td[i].reply & (0xFF<<16)) != 0) {
            ret = false;
            break;
        }
    }

    //Free td's
    KernelHeap::allignedFree(td);
    //Free queue head
    KernelHeap::allignedFree(queue);
    //Free packet
    KernelHeap::allignedFree(setupPacket);

    return ret;
}
bool UHCIController::BulkOut(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    uint32_t bufPhys = (uint32_t)VirtualMemoryManager::virtualToPhysical(bufPtr); 

    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    u_transferDescriptor_t* td = (u_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(u_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(u_transferDescriptor_t) * 10);

    //Allocate queue head
    uint32_t queuePhys;
    uhci_queue_head_t* queue = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t), 16, &queuePhys);
    MemoryOperations::memset(queue, 0, sizeof(uhci_queue_head_t));
    queue->horz_ptr = QUEUE_HEAD_T;
    queue->vert_ptr = tdPhys;
    
    int i = 0;
    int sz = len;
    //Transfer Descriptors depending on size of request
    while ((sz > 0) && (i<9)) {
        td[i].link_ptr = (i > 0 ? (td[i-1].link_ptr & ~0xF) : tdPhys) + sizeof(u_transferDescriptor_t);
        td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
        int t = ((sz <= packetSize) ? sz : packetSize);
        td[i].info = ((t-1)<<21) | ((i & 1) ? (1<<19) : 0)  | (endP<<15) | ((devAddress & 0x7F)<<8) | TOKEN_OUT;
        td[i].buff_ptr = bufPhys + (packetSize*i);
        sz -= t;
        i++;
    }
    td[i-1].reply |= (1<<24); //Enable IOC
    
    // make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);
    
    //Instert queue into Bulk list
    InsertQueue(queue, queuePhys, QUEUE_QBulk);
    
    // wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        RemoveQueue(queue, QUEUE_QBulk);
        return false;
    }

    Log(Info, "Frame: %d - USB transaction completed", inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111);
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    RemoveQueue(queue, QUEUE_QBulk);
    
    bool ret = true;
    // check the TD's for error
    for (int t = 0; t < i; t++) {
        if (((td[t].reply & (0xFF << 16)) != 0)) { //Check if any error bit is set
            ret = false;
            break;
        }
    }
    
    //Free td's
    KernelHeap::allignedFree(td);
    //Free queue head
    KernelHeap::allignedFree(queue);

    return ret;
}
bool UHCIController::BulkIn(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    //Create temporary buffer and clear it
    uint32_t returnBufPhys;
    uint8_t* returnBuf = (uint8_t*)KernelHeap::malloc(len, &returnBufPhys);
    MemoryOperations::memset(returnBuf, 0, len);
    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    u_transferDescriptor_t* td = (u_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(u_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(u_transferDescriptor_t) * 10);

    //Allocate queue head
    uint32_t queuePhys;
    uhci_queue_head_t* queue = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t), 16, &queuePhys);
    MemoryOperations::memset(queue, 0, sizeof(uhci_queue_head_t));
    queue->horz_ptr = QUEUE_HEAD_T;
    queue->vert_ptr = tdPhys;
    
    int i = 0;
    int sz = len;
    //Transfer Descriptors depending on size of request
    while ((sz > 0) && (i<9)) {
        td[i].link_ptr = (i > 0 ? (td[i-1].link_ptr & ~0xF) : tdPhys) + sizeof(u_transferDescriptor_t);
        td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
        int t = ((sz <= packetSize) ? sz : packetSize);
        td[i].info = ((t-1)<<21) | ((i & 1) ? (1<<19) : 0)  | (endP<<15) | ((devAddress & 0x7F)<<8) | TOKEN_IN;
        td[i].buff_ptr = returnBufPhys + (packetSize*i);
        sz -= t;
        i++;
    }
    td[i-1].reply |= (1<<24); //Enable IOC
    
    // make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);
    
    //Instert queue into list
    InsertQueue(queue, queuePhys, QUEUE_QBulk);
    
    // wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        RemoveQueue(queue, QUEUE_QBulk);
        return false;
    }

    Log(Info, "Frame: %d - USB transaction completed", inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111);
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    RemoveQueue(queue, QUEUE_QBulk);
    
    bool ret = true;
    // check the TD's for error
    for (int t = 0; t < i; t++) {
        if (((td[t].reply & (0xFF << 16)) != 0)) { //Check if any error bit is set
            ret = false;
            break;
        }
    }
    
    if(ret) {
        // copy the descriptor to the passed memory block
        MemoryOperations::memcpy(bufPtr, returnBuf, len);
    }
    
    //Free temporary buffer
    KernelHeap::free(returnBuf);
    //Free td's
    KernelHeap::allignedFree(td);
    //Free queue head
    KernelHeap::allignedFree(queue);

    return ret;
}
bool UHCIController::InterruptIn(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    u_transferDescriptor_t* td = (u_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(u_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(u_transferDescriptor_t) * 10);

    //Allocate queue head
    uint32_t queuePhys;
    uhci_queue_head_t* queue = (uhci_queue_head_t*)KernelHeap::allignedMalloc(sizeof(uhci_queue_head_t), 16, &queuePhys);
    MemoryOperations::memset(queue, 0, sizeof(uhci_queue_head_t));
    queue->horz_ptr = QUEUE_HEAD_T;
    queue->vert_ptr = tdPhys;
    
    int i = 0;
    int sz = len;
    //Transfer Descriptors depending on size of request
    while ((sz > 0) && (i<9)) {
        td[i].link_ptr = (i > 0 ? (td[i-1].link_ptr & ~0xF) : tdPhys) + sizeof(u_transferDescriptor_t);
        td[i].reply = (lsDevice ? (1<<26) : 0) | (3<<27) | (0x80 << 16);
        int t = ((sz <= packetSize) ? sz : packetSize);
        td[i].info = ((t-1)<<21) | ((i & 1) ? (1<<19) : 0)  | (endP<<15) | ((devAddress & 0x7F)<<8) | TOKEN_IN;
        td[i].buff_ptr = 0; //returnBufPhys + (packetSize*i);
        sz -= t;
        i++;
    }
    td[i-1].reply |= (1<<24); //Enable IOC
    
    // make sure status:int bit is clear
    outportw(pciDevice->portBase + UHCI_STATUS, 1);
    
    //Instert queue into list
    InsertQueue(queue, queuePhys, QUEUE_Q8);
    
    // wait for the IOC to happen
    int timeout = 10000; // 10 seconds
    while (!(inportw(pciDevice->portBase + UHCI_STATUS) & 1) && (timeout > 0)) {
        timeout--;
        System::pit->Sleep(1);
    }
    if (timeout == 0) {
        Log(Warning, "UHCI timed out.");
        RemoveQueue(queue, QUEUE_Q8);
        return false;
    }

    Log(Info, "Frame: %d - USB transaction completed", inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111);
    outportw(pciDevice->portBase + UHCI_STATUS, 1);  // acknowledge the interrupt
    
    RemoveQueue(queue, QUEUE_Q8);
    
    bool ret = true;
    // check the TD's for error
    for (int t = 0; t < i; t++) {
        if (((td[t].reply & (0xFF << 16)) != 0)) { //Check if any error bit is set
            ret = false;
            break;
        }
    }
    
    //Free td's
    KernelHeap::allignedFree(td);
    //Free queue head
    KernelHeap::allignedFree(queue);

    return ret;
}
uint32_t UHCIController::HandleInterrupt(uint32_t esp)
{
    uint16_t val = inportw(pciDevice->portBase + UHCI_STATUS);

    if(val == 0) // Interrupt came from another UHCI device
    {
        //Log(Info, "Interrupt came from another UHCI device!");
        return esp;
    }

    if (val & UHCI_STS_USBINT)
    {
        Log(Info, "UHCI: Transfer complete!");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_RESUME_DETECT); // reset interrupt
    }

    if (val & UHCI_STS_RESUME_DETECT)
    {
        Log(Info, "UHCI: Resume Detect");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_RESUME_DETECT); // reset interrupt
    }

    if (val & UHCI_STS_HCHALTED)
    {
        Log(Error, "UHCI: Host Controller Halted");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HCHALTED); // reset interrupt
    }

    if (val & UHCI_STS_HC_PROCESS_ERROR)
    {
        Log(Error, "UHCI: Host Controller Process Error");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HC_PROCESS_ERROR); // reset interrupt
    }

    if (val & UHCI_STS_USB_ERROR)
    {
        Log(Error, "UHCI: USB Error");
        int num = inportw(pciDevice->portBase + UHCI_FRAME_NUM) & 0b1111111111;
        Log(Info, "UHCI: Frame Base: %x Frame Num: %d Frame: %x", inportl(pciDevice->portBase + UHCI_FRAME_BASE), num, this->frameList[num]);
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_USB_ERROR); // reset interrupt
    }

    if (val & UHCI_STS_HOST_SYSTEM_ERROR)
    {
        Log(Error, "UHCI: Host System Error");
        outportw(pciDevice->portBase + UHCI_STATUS, UHCI_STS_HOST_SYSTEM_ERROR); // reset interrupt
    }

    return esp;
}

/////////
// USB Controller Functions
/////////

bool UHCIController::BulkIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    return BulkIn(device->uhciProperties.lowSpeedDevice, device->devAddress, device->endpoints[endP-1]->max_packet_size, endP, retBuffer, len);
}
bool UHCIController::BulkOut(USBDevice* device, void* sendBuffer, int len, int endP)
{
    return BulkOut(device->uhciProperties.lowSpeedDevice, device->devAddress, device->endpoints[endP-1]->max_packet_size, endP, sendBuffer, len);
}

bool UHCIController::ControlIn(USBDevice* device, void* target, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index)
{
    return ControlIn(target, device->uhciProperties.lowSpeedDevice, device->devAddress, device->uhciProperties.maxPacketSize, len, requestType, request, valueHigh, valueLow, index);
}
bool UHCIController::ControlOut(USBDevice* device, void* target, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index)
{
    return ControlOut(device->uhciProperties.lowSpeedDevice, device->devAddress, device->uhciProperties.maxPacketSize, len, requestType, request, valueHigh, valueLow, index);
}

bool UHCIController::InterruptIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    return InterruptIn(device->uhciProperties.lowSpeedDevice, device->devAddress, device->endpoints[endP-1]->max_packet_size, endP, retBuffer, len);
}