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

    
    /////////////////
    // Create Control list
    /////////////////
    uint32_t controlEDBase = (uint32_t)KernelHeap::allignedMalloc(sizeof(o_endpointDescriptor_t) * NUM_CONTROL_EDS, 16);
    for(int i = 0; i < NUM_CONTROL_EDS; i++) { //First Allocate all ED's
        this->controlEndpoints[i] = (o_endpointDescriptor_t*)(controlEDBase + sizeof(o_endpointDescriptor_t) * i);
        this->controlEndpointsPhys[i] = (uint32_t)VirtualMemoryManager::virtualToPhysical(this->controlEndpoints[i]);
        MemoryOperations::memset(this->controlEndpoints[i], 0, sizeof(o_endpointDescriptor_t));
    }
    //Now make it a linked list
    for (int i = 0; i < NUM_CONTROL_EDS; i++) {
        controlEndpoints[i]->nextED = i < 15 ? controlEndpointsPhys[i + 1] : 0x00000000;
        controlEndpoints[i]->flags = (1<<14); // sKip bit, should be 14 right? Not 13
    }


    /////////////////
    // Create Bulk list
    /////////////////
    uint32_t bulkEDBase = (uint32_t)KernelHeap::allignedMalloc(sizeof(o_endpointDescriptor_t) * NUM_BULK_EDS, 16);
    for(int i = 0; i < NUM_BULK_EDS; i++) { //First Allocate all ED's
        this->bulkEndpoints[i] = (o_endpointDescriptor_t*)(bulkEDBase + sizeof(o_endpointDescriptor_t) * i);
        this->bulkEndpointsPhys[i] = (uint32_t)VirtualMemoryManager::virtualToPhysical(this->bulkEndpoints[i]);
        MemoryOperations::memset(this->bulkEndpoints[i], 0, sizeof(o_endpointDescriptor_t));
    }
    //Now make it a linked list
    for (int i = 0; i < NUM_BULK_EDS; i++) {
        bulkEndpoints[i]->nextED = i < 15 ? bulkEndpointsPhys[i + 1] : 0x00000000;
        bulkEndpoints[i]->flags = (1<<14); // sKip bit, should be 14 right? Not 13
    }


    //Reset the root hub
    writeMemReg(regBase + OHCControl, 0x00000000);
    System::pit->Sleep(50);
    writeMemReg(regBase + OHCControl, 0x000000C0);  // suspend (stop reset)
    
    // set the Frame Interval, periodic start
    writeMemReg(regBase + OHCFmInterval, 0xA7782EDF);
    writeMemReg(regBase + OHCPeriodicStart, 0x00002A2F);

    // get the number of downstream ports
    numPorts = (uint8_t)(readMemReg(regBase + OHCRhDescriptorA) & 0x000000FF);
    Log(Info, "OHCI Found %d root hub ports.", numPorts);
    
    //Write the offset of our HCCA
    writeMemReg(regBase + OHCHCCA, hccaPhys);

    // write the offset of the control head ed
    writeMemReg(regBase + OHCControlHeadED, controlEndpointsPhys[0]);
    writeMemReg(regBase + OHCControlCurrentED, 0x00000000);
    writeMemReg(regBase + OHCBulkHeadED, bulkEndpointsPhys[0]);
    writeMemReg(regBase + OHCBulkCurrentED, 0x00000000);

    //Disallow interrupts (we poll the DoneHeadWrite bit instead)
    //writeMemReg(regBase + OHCInterruptDisable, (1<<31));
    writeMemReg(regBase + OHCInterruptEnable, (1<<0) | (0<<1) | (0<<2) | (1<<3) | (1<<4)| (0<<5) | (1<<6) | (1<<30) | (1<<31));
    
    // Start the controller  
    writeMemReg(regBase + OHCControl, (1<<4) | (1<<5) | (1<<7) | (1<<9) | (1<<10));  // CLE & operational
    
    //Set port power switching
    uint32_t val = readMemReg(regBase + OHCRhDescriptorA);
    //Read Power On To Power Good Time (Is in units of 2 mS)
    uint16_t potpgt = (uint16_t) (((val >> 24) * 2) + 2);  // plus two to make sure we wait long enough
    writeMemReg(regBase + OHCRhDescriptorA, ((val & OHCRhDescriptorA_MASK) & ~(1<<9)) | (1<<8));

    // loop through the ports
    for (int i = 0; i < numPorts; i++) {
        // power the port
        writeMemReg(regBase + OHCRhPortStatus + (i * 4), (1<<8));
        System::pit->Sleep(potpgt);
        
        val = readMemReg(regBase + OHCRhPortStatus + (i * 4));
        if (val & 1) {
            if (!ResetPort(i))
                continue;
            
            SetupNewDevice(i);
        }
    }
}
void OHCIController::SetupNewDevice(uint8_t port)
{
    static int dev_address = 1;
    struct DEVICE_DESC devDesc;

    // port has been reset, and is ready to be used
    bool ls_device = (readMemReg(regBase + OHCRhPortStatus + (port * 4)) & (1<<9)) ? 1 : 0;
    Log(Info, "OHCI, Found Device at port %d, low speed = %b", port, ls_device);

    // Some devices will only send the first 8 bytes of the device descriptor
    // while in the default state.  We must request the first 8 bytes, then reset
    // the port, set address, then request all 18 bytes.
    bool good_ret = ControlIn(&devDesc, ls_device, 0, 8, 8, STDRD_GET_REQUEST, GET_DESCRIPTOR, DEVICE);
    if (good_ret) {                
        //Reset the port again
        if (!ResetPort(port))
            return;
       
        //Set address
        good_ret = ControlOut(ls_device, 0, devDesc.max_packet_size, 0, STDRD_SET_REQUEST, SET_ADDRESS, 0, dev_address);
        if (!good_ret) {
            Log(Error, "Error when trying to set device address to %d", dev_address);
            return;
        }

        //Create Device
        USBDevice* newDev = new USBDevice();
        newDev->controller = this;
        newDev->devAddress = dev_address;
        newDev->portNum = port;
        newDev->ohciProperties.desc_mps = devDesc.max_packet_size;
        newDev->ohciProperties.ls_device = ls_device;
        
        System::usbManager->AddDevice(newDev);
        
        dev_address++;
    }
}
void OHCIController::ControllerChecksThread()
{
    for (int i = 0; i < numPorts; i++) 
    {        
        uint32_t portSts = readMemReg(regBase + OHCRhPortStatus + (i * 4));
        if(portSts & (1<<16)) //Port Connection Change Bit
        {
            writeMemReg(regBase + OHCRhPortStatus + (i * 4), (1<<16));
            Log(Info, "OHCI Port %d Connection change, now %s", i, (portSts & (1<<0)) ? "Connected" : "Not Connected");

            if((portSts & (1<<0)) == 1) { //Connected
                if(ResetPort(i))
                    SetupNewDevice(i);
            }
            else { //Not Connected
                System::usbManager->RemoveDevice(this, i);
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
bool OHCIController::ControlOut(const bool lsDevice, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) 
{
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
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 2, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 2);
    
    //Create the setup td
    td[0].flags = (14<<28) | (2<<24) | (7<<21) | (TD_DP_SETUP<<19);
    td[0].curBufPtr = setupPacketPhys;
    td[0].nextTd = tdPhys + sizeof(o_transferDescriptor_t);
    td[0].bufEnd = td[0].curBufPtr + 7;
    
    //Create the status td
    td[1].flags = (14<<28) | (3<<24) | (0<<21) | (TD_DP_IN<<19);
    td[1].curBufPtr = 0;
    td[1].nextTd = tdPhys + (sizeof(o_transferDescriptor_t) * 2);
    td[1].bufEnd = 0;
    
    //Create the ED, using one already in the control list
    this->controlEndpoints[0]->flags = (packetSize << 16) | (0 << 15) | (0 << 14) | (lsDevice ? (1<<13) : 0) | (0 << 11) | (ENDP_CONTROL << 7) | (devAddress & 0x7F);
    this->controlEndpoints[0]->tailp = tdPhys + sizeof(o_transferDescriptor_t) * 2;
    this->controlEndpoints[0]->headp = tdPhys;
    
    //Set ControlListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<1));
    
    //Wait for TD completion
    for(int c = 0; c <= 1; c++) {
        int timeOut = OHCI_TD_TIMEOUT;
        while((((td[c].flags & 0xF0000000) >> 28) == 14) && (timeOut > 0)) //This TD Has not been accesed
        { 
            timeOut--; 
            System::pit->Sleep(1); 
        }
        //Log(Info, "OHCI TD %d Finished by controller err=%d timeout=%d", c, ((td[c].flags & 0xF0000000) >> 28), timeOut);
    }

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
    //Free Packet
    KernelHeap::allignedFree(setupPacket);

    return ret;
}

bool OHCIController::ControlIn(void* targ, const bool lsDevice, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) {
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
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 10);
    
    //Create the setup td
    td[0].flags = (14<<28) | (0 << 26) | (2 << 24) | (7<<21) | (TD_DP_SETUP << 19);
    td[0].curBufPtr = requestPacketPhys;
    td[0].nextTd = tdPhys + sizeof(o_transferDescriptor_t);
    td[0].bufEnd = td[0].curBufPtr + 7;
    
    //Create the rest of the in td's
    int i = 1; int p = 0; int t = 1;
    int cnt = len;
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
    this->controlEndpoints[0]->flags = (packetSize << 16) | (0 << 15) | (0 << 14) | (lsDevice ? (1<<13) : 0) | (0 << 11) | (ENDP_CONTROL << 7) | (devAddress & 0x7F);
    this->controlEndpoints[0]->tailp = td[i].nextTd;
    this->controlEndpoints[0]->headp = tdPhys;

    //Set ControlListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<1));
    
    //Wait for TD completion
    for(int c = 0; c <= i; c++) {
        int timeOut = OHCI_TD_TIMEOUT;
        while((((td[c].flags & 0xF0000000) >> 28) == 14) && (timeOut > 0)) //This TD Has not been accesed
        { 
            timeOut--; 
            System::pit->Sleep(1); 
        }
        //Log(Info, "OHCI TD %d Finished by controller err=%d timeout=%d", c, ((td[c].flags & 0xF0000000) >> 28), timeOut);
    }

    //Reset Used ED
    this->controlEndpoints[0]->flags = (1<<14);
    this->controlEndpoints[0]->tailp = 0;
    this->controlEndpoints[0]->headp = 0;
    
    // calculate how many tds to check.
    cnt = len;
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
        MemoryOperations::memcpy(targ, returnBuf, len);
    }
    
    //Free temporary buffer
    KernelHeap::free(returnBuf);
    //Free td's
    KernelHeap::allignedFree(td);
    //Free packet
    KernelHeap::allignedFree(requestPacket);
    
    return ret;
}
bool OHCIController::BulkOut(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    uint32_t bufPhys = (uint32_t)VirtualMemoryManager::virtualToPhysical(bufPtr); 

    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 10);
    
    //Create the rest of the in td's
    int i = 0; int p = 0; int toggle = 1;
    int cnt = len;
    while (cnt > 0) {
        td[i].flags = (14<<28) | (0 << 26) | ((2 | (toggle & 1)) << 24) | (7<<21) | (0 << 19) /*Dir from ED*/;
        td[i].curBufPtr = bufPhys + p;
        td[i].nextTd = ((i > 0) ? td[i-1].nextTd : tdPhys) + sizeof(o_transferDescriptor_t);
        td[i].bufEnd = td[i].curBufPtr + ((cnt > packetSize) ? (packetSize - 1) : (cnt - 1));
        toggle ^= 1;
        p += packetSize;
        i++;
        cnt -= packetSize;
    }
    
    //Create the ED, using one already in the bulk list
    this->bulkEndpoints[0]->flags = (packetSize << 16) | (0 << 15) | (0 << 14) | (lsDevice ? (1<<13) : 0) | (TD_DP_OUT << 11) | (endP << 7) | (devAddress & 0x7F);
    this->bulkEndpoints[0]->tailp = tdPhys + sizeof(o_transferDescriptor_t) * i;
    this->bulkEndpoints[0]->headp = tdPhys;
    
    //Set BulkListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<2));
    
    //Wait for TD completion
    for(int c = 0; c < i; c++) {
        int timeOut = OHCI_TD_TIMEOUT;
        while((((td[c].flags & 0xF0000000) >> 28) == 14) && (timeOut > 0)) //This TD Has not been accesed
        { 
            timeOut--; 
            System::pit->Sleep(1); 
        }
        //Log(Info, "OHCI TD %d Finished by controller err=%d timeout=%d", c, ((td[c].flags & 0xF0000000) >> 28), timeOut);
        if(timeOut == 0)
            break;
    }

    //Reset Used ED
    this->bulkEndpoints[0]->flags = (1<<14);
    this->bulkEndpoints[0]->tailp = 0;
    this->bulkEndpoints[0]->headp = 0;
    
    bool ret = true;
    for (int c = 0; c < i; c++) {
        if ((td[c].flags & 0xF0000000) != 0) {
            ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", c, (td[c].flags & 0xF0000000) >> 28);
            break;
        }
    }
    
    //Free td's
    KernelHeap::allignedFree(td);
    return ret;
}
bool OHCIController::BulkIn(const bool lsDevice, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    //Create temporary buffer and clear it
    uint32_t returnBufPhys;
    uint8_t* returnBuf = (uint8_t*)KernelHeap::malloc(len, &returnBufPhys);
    MemoryOperations::memset(returnBuf, 0, len);
    
    //Allocate Transfer Descriptors
    uint32_t tdPhys;
    o_transferDescriptor_t* td = (o_transferDescriptor_t*)KernelHeap::allignedMalloc(sizeof(o_transferDescriptor_t) * 10, 16, &tdPhys);
    MemoryOperations::memset(td, 0, sizeof(o_transferDescriptor_t) * 10);
    
    //Create the rest of the in td's
    int i = 0; int p = 0; int t = 1;
    int cnt = len;
    while (cnt > 0) {
        td[i].flags = (14<<28) | (0 << 26) | ((2 | (t & 1)) << 24) | (7<<21) | (0 << 19) /*Dir from ED*/  | (1<<18) /*Buffer Rounding*/;
        td[i].curBufPtr = returnBufPhys + p;
        td[i].nextTd = ((i > 0) ? td[i-1].nextTd : tdPhys) + sizeof(o_transferDescriptor_t);
        td[i].bufEnd = td[i].curBufPtr + ((cnt > packetSize) ? (packetSize - 1) : (cnt - 1));
        t ^= 1;
        p += packetSize;
        i++;
        cnt -= packetSize;
    }
    
    //Create the ED, using one already in the bulk list
    this->bulkEndpoints[0]->flags = (packetSize << 16) | (0 << 15) | (0 << 14) | (lsDevice ? (1<<13) : 0) | (TD_DP_IN << 11) | (endP << 7) | (devAddress & 0x7F);
    this->bulkEndpoints[0]->tailp = tdPhys + sizeof(o_transferDescriptor_t) * i;
    this->bulkEndpoints[0]->headp = tdPhys;

    //Set BulklListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<2));
    
    //Wait for TD completion
    for(int c = 0; c < i; c++) {
        int timeOut = OHCI_TD_TIMEOUT;
        while((((td[c].flags & 0xF0000000) >> 28) == 14) && (timeOut > 0)) //This TD Has not been accesed
        { 
            timeOut--; 
            System::pit->Sleep(1); 
        }
        //Log(Info, "OHCI TD %d Finished by controller err=%d timeout=%d", c, ((td[c].flags & 0xF0000000) >> 28), timeOut);
    }

    //Reset Used ED
    this->bulkEndpoints[0]->flags = (1<<14);
    this->bulkEndpoints[0]->tailp = 0;
    this->bulkEndpoints[0]->headp = 0;
    
    bool ret = true;
    for (int c = 0; c < i; c++) {
        if ((td[c].flags & 0xF0000000) != 0) {
            uint8_t err = (td[c].flags & 0xF0000000) >> 28;
            ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", c, err);
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
        Log(Info, "OHCI: Scheduling overrun.");
    }

    if(val & (1<<1))
    {
        Log(Info, "OHCI: Writeback Done Head");
    }

    if (val & (1<<3))
    {
        Log(Info, "OHCI: Resume detected.");
    }

    if (val & (1<<4))
    {
        Log(Info, "OHCI: Unrecoverable HC error.");
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
    return ControlIn(dev_desc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, sizeof(struct DEVICE_DESC), STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::DEVICE);
}
bool OHCIController::GetStringDescriptor(struct STRING_DESC* stringDesc, USBDevice* device, uint16_t index, uint16_t lang)
{
    if(!ControlIn(stringDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, 2, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, index, lang))
        return false;
        
    int totalSize = stringDesc->len;
    return ControlIn(stringDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, totalSize, STDRD_GET_REQUEST, DeviceRequest::GET_DESCRIPTOR, DescriptorTypes::STRING, index, lang);
}
uint8_t* OHCIController::GetConfigDescriptor(USBDevice* device)
{
    struct CONFIG_DESC confDesc;
    MemoryOperations::memset(&confDesc, 0, sizeof(struct CONFIG_DESC));

    if(!ControlIn(&confDesc, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, sizeof(struct CONFIG_DESC), STDRD_GET_REQUEST, GET_DESCRIPTOR, CONFIG))
        return 0;
    
    int totalSize = confDesc.tot_len;
    uint8_t* buffer = new uint8_t[totalSize];
    MemoryOperations::memset(buffer, 0, totalSize);

    if(!ControlIn(buffer, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, totalSize, STDRD_GET_REQUEST, GET_DESCRIPTOR, CONFIG))
        return 0;
    
    return buffer;
}
bool OHCIController::SetConfiguration(USBDevice* device, uint8_t config) 
{
    return ControlOut(device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, 0, STDRD_SET_REQUEST, SET_CONFIGURATION, 0, config);
}
int OHCIController::GetMaxLuns(USBDevice* device)
{
    uint8_t ret;
    if(ControlIn(&ret, device->ohciProperties.ls_device, device->devAddress, device->ohciProperties.desc_mps, 1, DEV_TO_HOST | REQ_TYPE_CLASS | RECPT_INTERFACE, GET_MAX_LUNS))
        return ret;
    return 0;
}
bool OHCIController::BulkIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    return BulkIn(device->ohciProperties.ls_device, device->devAddress, device->endpoints[endP-1]->max_packet_size, endP, retBuffer, len);
}
bool OHCIController::BulkOut(USBDevice* device, void* sendBuffer, int len, int endP)
{
    return BulkOut(device->ohciProperties.ls_device, device->devAddress, device->endpoints[endP-1]->max_packet_size, endP, sendBuffer, len);
}