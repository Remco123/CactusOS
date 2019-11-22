#include <system/drivers/usb/controllers/ohci.h>
#include <system/system.h>
#include <system/drivers/usb/usbdefs.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

OHCIController::OHCIController(PCIDevice* device)
: USBController(OHCI), Driver("OHCI USB Controller", "Controller for a OHCI device"),
  InterruptHandler(IDT_INTERRUPT_OFFSET + device->interrupt) 
{
    this->pciDevice = device;
}

bool OHCIController::Initialize()
{
    BaseAddressRegister BAR0 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 0);
    if(BAR0.type == InputOutput)
        return false; //We only want memory mapped controllers

    uint32_t virtIOArea = (uint32_t)KernelHeap::allignedMalloc(pageRoundUp(BAR0.size), PAGE_SIZE);
    MemoryOperations::memset((void*)virtIOArea, 0, pageRoundUp(BAR0.size));
    //Save Register memory base
    this->regBase = virtIOArea;

    //Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, 0x0006);

    //Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)BAR0.address, (void*)virtIOArea, pageRoundUp(BAR0.size), true, true);

    //First, read the version register and check for version 1.0
    if ((readMemReg(regBase + OHCRevision) & 0xFF) != 0x10)
        return false;

    //Reset the controller, returning false after 30mS if it doesn't reset
    uint8_t timeout = 30;
    writeMemReg(regBase + OHCCommandStatus, (1 << 0));
    while (readMemReg(regBase + OHCCommandStatus) & (1 << 0)) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            return false;
    }

    //See if the controllers funtional state is in the suspend state
    if ((readMemReg(regBase + OHCControl) & 0xC0) != 0xC0)
        return false;
    
    //See if the frameinterval register is 0x2EDF
    if ((readMemReg(regBase + OHCFmInterval) & 0x00003FFF) != 0x2EDF)
        return false;

    //Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void OHCIController::Setup()
{
    //First get allignment of HCCA
    //Write 0xFFFFFFFF to register
    writeMemReg(regBase + OHCHCCA, 0xFFFFFFFF);
    //Read back modified address
    uint32_t ret = readMemReg(regBase + OHCHCCA);
    //Allocate Memory for HCCA
    this->hcca = (OHCI_HCCA*)KernelHeap::allignedMalloc(131072, ~ret + 1, &hccaPhys);
    if(this->hcca == 0){
        Log(Error, "Error Allocating HCCA Memory, Alignment = %x", ~ret + 1);
        return;
    }
    //Clear structure
    MemoryOperations::memset(this->hcca, 0x0, sizeof(OHCI_HCCA));
        
    //Create Control list
    uint32_t edBase = (uint32_t)KernelHeap::allignedMalloc(sizeof(o_endpointDescriptor_t) * NUM_CONTROL_EDS, 16);
    for(int i = 0; i < NUM_CONTROL_EDS; i++) { //First Allocate all ED's
        this->controlEndpoints[i] = (o_endpointDescriptor_t*)(edBase + sizeof(o_endpointDescriptor_t) * i);
        this->controlEndpointsPhys[i] = (uint32_t)VirtualMemoryManager::virtualToPhysical(this->controlEndpoints[i]);
        MemoryOperations::memset(this->controlEndpoints[i], 0, sizeof(o_endpointDescriptor_t));
    }
    //Now make it a linked list
    for (int i = 0; i < NUM_CONTROL_EDS; i++) {
        controlEndpoints[i]->nextED = i < 15 ? controlEndpointsPhys[i + 1] : 0x00000000;
        controlEndpoints[i]->flags = (1<<14); // sKip bit, should be 14 right? Not 13
    }

    //Reset the root hub
    writeMemReg(regBase + OHCControl, 0x00000000);
    System::pit->Sleep(50);
    writeMemReg(regBase + OHCControl, 0x000000C0);  // suspend (stop reset)
    
    // set the Frame Interval, periodic start
    writeMemReg(regBase + OHCFmInterval, 0xA7782EDF);
    writeMemReg(regBase + OHCPeriodicStart, 0x00002A2F);

    // get the number of downstream ports
    uint8_t numPorts = (uint8_t)(readMemReg(regBase + OHCRhDescriptorA) & 0x000000FF);
    Log(Info, "OHCI Found %d root hub ports.", numPorts);
    
    //Write the offset of our HCCA
    writeMemReg(regBase + OHCHCCA, hccaPhys);

    // write the offset of the control head ed
    writeMemReg(regBase + OHCControlHeadED, controlEndpointsPhys[0]);
    writeMemReg(regBase + OHCControlCurrentED, 0x00000000);
    writeMemReg(regBase + OHCBulkCurrentED, 0x00000000);

    //Disallow interrupts (we poll the DoneHeadWrite bit instead)
    //writeMemReg(regBase + OHCInterruptDisable, (1<<31));
    writeMemReg(regBase + OHCInterruptEnable, (1<<0) | (1<<1) | (0<<2) | (1<<3) | (1<<4)| (0<<5) | (1<<6) | (1<<30) | (1<<31));
    
    // Start the controller  
    writeMemReg(regBase + OHCControl, 0x00000690);  // CLE & operational
    
    //Set port power switching
    uint32_t val = readMemReg(regBase + OHCRhDescriptorA);
    //Read Power On To Power Good Time (Is in units of 2 mS)
    uint16_t potpgt = (uint16_t) (((val >> 24) * 2) + 2);  // plus two to make sure we wait long enough
    writeMemReg(regBase + OHCRhDescriptorA, ((val & OHCRhDescriptorA_MASK) & ~(1<<9)) | (1<<8));

    // loop through the ports
    int dev_address = 1;
    struct DEVICE_DESC devDesc;
    for (int i = 0; i < numPorts; i++) {
        // power the port
        writeMemReg(regBase + OHCRhPortStatus + (i * 4), (1<<8));
        System::pit->Sleep(potpgt);
        
        val = readMemReg(regBase + OHCRhPortStatus + (i * 4));
        if (val & 1) {
            if (!ResetPort(i))
                continue;
            
            // port has been reset, and is ready to be used
            bool ls_device = (readMemReg(regBase + OHCRhPortStatus + (i * 4)) & (1<<9)) ? 1 : 0;
            Log(Info, "OHCI, Found Device at port %d, low speed = %b", i, ls_device);

            // Some devices will only send the first 8 bytes of the device descriptor
            // while in the default state.  We must request the first 8 bytes, then reset
            // the port, set address, then request all 18 bytes.
            bool good_ret = RequestDesc(&devDesc, ls_device, 0, 8, 8, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE);
            if (good_ret) {                
                //Reset the port again
                if (!ResetPort(i))
                    continue;
               
                //Set address
                good_ret = SetAddress(dev_address, ls_device);
                if (!good_ret) {
                    Log(Error, "Error when trying to set device address to %d", dev_address);
                    continue;
                }

                //Create Device
                USBDevice* newDev = new USBDevice();
                newDev->controller = this;
                newDev->devAddress = dev_address;
                newDev->portNum = i;
                newDev->ohciProperties.desc_mps = devDesc.max_packet_size;
                newDev->ohciProperties.ls_device = ls_device;
                
                System::usbManager->AddDevice(newDev);
                
                dev_address++;
            }
        }
    }
}
bool OHCIController::ResetPort(uint8_t port) {
    int timeout = 30;
    writeMemReg(regBase + OHCRhPortStatus + (port * 4), (1<<4)); // reset port
    while ((readMemReg(regBase + OHCRhPortStatus + (port * 4)) & (1<<20)) == 0) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            break;
    }
    if (timeout == 0) {
        Log(Warning, "Port %d did not reset after 30mS.", port);
        return false;
    }
    System::pit->Sleep(USB_TRSTRCY);  // hold for USB_TRSTRCY ms (reset recovery time)
    
    // clear status change bits
    writeMemReg(regBase + OHCRhPortStatus + (port * 4), (0x1F<<16));
    
    return true;
}
bool OHCIController::WaitForInterrupt()
{
    // clear all the bits in the interrupt status register
    writeMemReg(regBase + OHCInterruptStatus, (1<<30) | 0x7F);
    
    // set ControlListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<1));

    //TODO: Find alternative to this delay
    System::pit->Sleep(5);
    
    // wait for bit 1 to be set in the status register
    int timeout = 2000; // 2 seconds
    while ((readMemReg(pciDevice->portBase + OHCInterruptStatus) & (1<<1)) == 0) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            break;
    }
    if (timeout == 0) {
        Log(Warning, "Bit 1 in the HCInterruptStatus register never was set.");
        return false;
    }
    
    // here is were we would read the HCCA.DoneHead field, and go through the list
    //  to see if there were any errors.
    // For the purpose of this example, we will not use the HCCA.DoneHead, but just
    //  scroll through our transfer descriptors in our local stack.
    
    // clear the bit for next time
    writeMemReg(regBase + OHCInterruptStatus, (1<<1));

    return true;
}
bool OHCIController::SetAddress(int dev_address, bool ls_device) 
{
    //Create setupPacket
    REQUEST_PACKET setupPacket __attribute__((aligned(16)));
    {
        setupPacket.request_type = STDRD_SET_REQUEST;
        setupPacket.request = SET_ADDRESS;
        setupPacket.value = dev_address;
        setupPacket.index = 0;
        setupPacket.length = 0;
    }

    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 2, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 2);
    
    //Create the setup td
    td[0].flags = (14<<28) | (2<<24) | (7<<21) | (0<<19);
    td[0].curBufPtr = (uint32_t)virt2phys((uint32_t)&setupPacket);
    td[0].nextTd = tdPhys + sizeof(o_transferDescriptor_t);
    td[0].bufEnd = td[0].curBufPtr + 7;
    
    //Create the status td
    td[1].flags = (14<<28) | (3<<24) | (0<<21) | (2<<19);
    td[1].curBufPtr = 0;
    td[1].nextTd = tdPhys + (sizeof(o_transferDescriptor_t) * 2);
    td[1].bufEnd = 0;
    
    //Create the ED, using one already in the control list
    this->controlEndpoints[0]->flags = 0x00080000 | ((ls_device) ? (1<<13) : 0);
    this->controlEndpoints[0]->tailp = tdPhys + sizeof(o_transferDescriptor_t) * 2;
    this->controlEndpoints[0]->headp = tdPhys;

    //Wait for interrupt from controller
    WaitForInterrupt();

    //Reset Used ED
    this->controlEndpoints[0]->flags = (1<<14);
    this->controlEndpoints[0]->tailp = 0;
    this->controlEndpoints[0]->headp = 0;
    
    bool ret = true;
    for (int i = 0; i < 2; i++) {
        if ((td[i].flags & 0xF0000000) != 0) {
            ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", i, (td[i].flags & 0xF0000000) >> 28);
            break;
        }
    }
    
    //Free td's
    KernelHeap::allignedFree(td);

    return ret;
}

bool OHCIController::RequestDesc(void* devDesc, const bool lsDevice, const int devAddress, const int packetSize, const int size, const uint8_t requestType, const uint8_t request, const uint16_t valueLow, const uint16_t valueHigh, const uint16_t index) {
    //Create Request Packet
    REQUEST_PACKET requestPacket __attribute__((aligned(16)));
    {
        requestPacket.request_type = requestType;
        requestPacket.request = request;
        requestPacket.value = (valueHigh >> 8) | (valueLow << 8);
        requestPacket.index = index;
        requestPacket.length = size;
    }

    //Create temporary buffer and clear it
    uint32_t returnBufPhys;
    uint8_t* returnBuf = (uint8_t*)KernelHeap::malloc(size, &returnBufPhys);
    MemoryOperations::memset(returnBuf, 0, size);
    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 10);
    
    //Create the setup td
    td[0].flags = (14<<28) | (0 << 26) | (2 << 24) | (7<<21) | (TD_DP_SETUP << 19);
    td[0].curBufPtr = (uint32_t)virt2phys((uint32_t)&requestPacket);
    td[0].nextTd = tdPhys + sizeof(o_transferDescriptor_t);
    td[0].bufEnd = td[0].curBufPtr + 7;
    
    //Create the rest of the in td's
    int i = 1; int p = 0; int t = 1;
    int cnt = size;
    while (cnt > 0) {
        td[i].flags = (14<<28) | (0 << 26) | ((2 | (t & 1)) << 24) | (7<<21) | (TD_DP_IN << 19);
        td[i].curBufPtr = returnBufPhys + p;
        td[i].nextTd = td[i-1].nextTd + sizeof(o_transferDescriptor_t);
        td[i].bufEnd = td[i].curBufPtr + ((cnt > packetSize) ? (packetSize - 1) : (cnt - 1));
        t ^= 1;
        p += packetSize;
        i++;
        cnt -= packetSize;
    }
    
    //Create the status td
    td[i].flags = (14<<28) | (0 << 26) | (3 << 24) | (0<<21) | (TD_DP_OUT << 19);
    td[i].curBufPtr = 0;
    td[i].nextTd = tdPhys + (sizeof(o_transferDescriptor_t) * i) + sizeof(o_transferDescriptor_t);
    td[i].bufEnd = 0;
    
    //Create the ED, using one already in the control list
    this->controlEndpoints[0]->flags = (packetSize << 16) | (0 << 15) | (0 << 14) | (lsDevice ? (1<<13) : 0) | (0 << 11) | (0 << 7) | (devAddress & 0x7F);
    this->controlEndpoints[0]->tailp = td[i].nextTd;
    this->controlEndpoints[0]->headp = tdPhys;
    
    //Wait for interrupt from controller
    WaitForInterrupt();

    //Reset Used ED
    this->controlEndpoints[0]->flags = (1<<14);
    this->controlEndpoints[0]->tailp = 0;
    this->controlEndpoints[0]->headp = 0;
    
    // calculate how many tds to check.
    cnt = size;
    if (cnt > 0)
        cnt /= 8;
    cnt += 2;
    
    bool ret = true;
    for (int i = 0; i < cnt; i++) {
        if ((td[i].flags & 0xF0000000) != 0) {
            uint8_t err = (td[i].flags & 0xF0000000) >> 28;
            ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", i, err);
            break;
        }
    }

    if(ret) {
        // copy the descriptor to the passed memory block
        MemoryOperations::memcpy(devDesc, returnBuf, size);
    }
    
    //Free temporary buffer
    KernelHeap::free(returnBuf);
    //Free td's
    KernelHeap::allignedFree(td);
    
    return ret;
}
uint32_t OHCIController::HandleInterrupt(uint32_t esp)
{
    uint32_t val = readMemReg(regBase + OHCInterruptStatus);
    if(val == 0)
        return esp;

    writeMemReg(regBase + OHCInterruptStatus, val); // reset interrupts

    if (!((val & (1<<2)) || (val & (1<<6))))
    {
        Log(Info, "USB OHCI %d: ", this->hcca->HccaFrameNumber);
    }

    if (val & (1<<0))
    {
        Log(Info, "Scheduling overrun.");
    }

    if (val & (1<<3))
    {
        Log(Info, "Resume detected.");
    }

    if (val & (1<<4))
    {
        Log(Info, "Unrecoverable HC error.");
    }

    if (val & (1<<5))
    {
        Log(Info, "OHCI, Frame number overflow");
    }
    if ((val & (1<<6)))
    {
        Log(Info, "OHCI, Root hub status change");
    }
    if (val & (1<<30))
    {
        Log(Info, "OHCI, ownership change");
    }

    return esp;
}
/////////
// USB Controller Functions
/////////
bool OHCIController::GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device)
{
    return RequestDesc(dev_desc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, sizeof(struct DEVICE_DESC), STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE);
}
bool OHCIController::GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang)
{
    if(!RequestDesc(stringDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, 2, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, lang, index))
        return false;
        
    int totalSize = stringDesc->len;
    return RequestDesc(stringDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, totalSize, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, lang, index);
}
uint8_t* OHCIController::GetConfigDescriptor(USBDevice* device)
{
    struct CONFIG_DESC confDesc;
    MemoryOperations::memset(&confDesc, 0, sizeof(struct CONFIG_DESC));

    if(!RequestDesc(&confDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, sizeof(struct CONFIG_DESC), STDRD_GET_REQUEST, GET_DESCRIPTOR, CONFIG))
        return 0;
    
    int totalSize = confDesc.tot_len;
    uint8_t* buffer = new uint8_t[totalSize];
    MemoryOperations::memset(buffer, 0, totalSize);

    if(!RequestDesc(buffer, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, totalSize, STDRD_GET_REQUEST, GET_DESCRIPTOR, CONFIG))
        return 0;
    
    return buffer;
}
bool OHCIController::SetConfiguration(USBDevice* device, uint8_t config) 
{
//Create setupPacket
    REQUEST_PACKET setupPacket __attribute__((aligned(16)));
    {
        setupPacket.request_type = STDRD_SET_REQUEST;
        setupPacket.request = SET_CONFIGURATION;
        setupPacket.value = config;
        setupPacket.index = 0;
        setupPacket.length = 0;
    }

    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 2, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 2);
    
    //Create the setup td
    td[0].flags = (14<<28) | (2<<24) | (7<<21) | (0<<19);
    td[0].curBufPtr = (uint32_t)virt2phys((uint32_t)&setupPacket);
    td[0].nextTd = tdPhys + sizeof(o_transferDescriptor_t);
    td[0].bufEnd = td[0].curBufPtr + 7;
    
    //Create the status td
    td[1].flags = (14<<28) | (3<<24) | (0<<21) | (2<<19);
    td[1].curBufPtr = 0;
    td[1].nextTd = tdPhys + (sizeof(o_transferDescriptor_t) * 2);
    td[1].bufEnd = 0;
    
    //Create the ED, using one already in the control list
    this->controlEndpoints[0]->flags = 0x00080000 | (device->ohciProperties.desc_mps << 16) | ((device->ohciProperties.ls_device) ? (1<<13) : 0) | (device->devAddress & 0x7F);
    this->controlEndpoints[0]->tailp = tdPhys + sizeof(o_transferDescriptor_t) * 2;
    this->controlEndpoints[0]->headp = tdPhys;

    //Wait for interrupt from controller
    WaitForInterrupt();

    //Reset Used ED
    this->controlEndpoints[0]->flags = (1<<14);
    this->controlEndpoints[0]->tailp = 0;
    this->controlEndpoints[0]->headp = 0;
    
    bool ret = true;
    for (int i = 0; i < 2; i++) {
        if ((td[i].flags & 0xF0000000) != 0) {
            ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", i, (td[i].flags & 0xF0000000) >> 28);
            break;
        }
    }
    
    //Free td's
    KernelHeap::allignedFree(td);

    return ret;
}