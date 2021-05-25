#include <system/drivers/usb/controllers/ehci.h>
#include <system/drivers/usb/usbdefs.h>
#include <system/memory/deviceheap.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

EHCIController::EHCIController(PCIDevice* device)
: USBController(EHCI), Driver("EHCI USB Controller", "Controller for a EHCI device")
, InterruptHandler(IDT_INTERRUPT_OFFSET + device->interrupt) 
{
    this->pciDevice = device;
}

bool EHCIController::Initialize()
{
    BaseAddressRegister BAR0 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 0);
    if(BAR0.type == InputOutput)
        return false; // We only want memory mapped controllers

    uint32_t memStart = pageRoundDown((uint32_t)BAR0.address); // Assuming 32-Bit address
    uint32_t memEnd = pageRoundUp((uint32_t)BAR0.address + BAR0.size);
    
    // Allocate virtual chuck of memory that we can use for device
    this->regBase = DeviceHeap::AllocateChunk(memEnd - memStart) + ((uint32_t)BAR0.address % PAGE_SIZE);

    // Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)memStart, (void*)this->regBase, memEnd - memStart, true, true);

    Log(Info, "[EHCI] Controller Memory addres %x -> Memory Mapped: Start=%x End=%x Size=%x", (uint32_t)BAR0.address, memStart, memEnd, memEnd - memStart);
    Log(Info, "[EHCI] regBase -> %x", this->regBase);

    // Enable BUS Mastering
    uint32_t busMasterTemp = System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04);
    busMasterTemp &= ~PCI_CMDREG_IO;
    busMasterTemp |= PCI_CMDREG_MEM | PCI_CMDREG_BM;
    Log(Info, "[EHCI] PCI Command before and after -> %b %b", System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04), busMasterTemp);
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, busMasterTemp);

    // Calculate the operational base
    // Must be before any ReadOpReg() or WriteOpReg() calls
    operRegsOffset = (uint8_t)readMemReg(this->regBase + EHCI_CAPS_CapLength);
    Log(Info, "[EHCI] operRegsOffset = %b", operRegsOffset);

    uint32_t structuralParams = readMemReg(this->regBase + EHCI_CAPS_HCSParams);
    uint32_t capabilityParams = readMemReg(this->regBase + EHCI_CAPS_HCCParams);
    Log(Info, "[EHCI] structuralParams = %b", structuralParams);
    Log(Info, "[EHCI] capabilityParams = %b", capabilityParams);

    // Get num_ports from EHCI's HCSPARAMS register
    numPorts = (uint8_t)(structuralParams & 0x0F);  // At least 1 and no more than 15
    Log(Info, "EHCI Found %d root hub ports.", numPorts);

    // Turn off legacy support for Keyboard and Mice
    if (!this->StopLegacy(capabilityParams)) {
        Log(Error, "[EHCI] BIOS did not release Legacy support...");
        return false;
    }

    // Disable interrupts
    WriteOpReg(EHCI_OPS_USBInterrupt, 0x0);

    // Do what the function says
    if (!this->ResetController()) {
        Log(Error, "[EHCI] Could not reset controller");
        return false;
    }

    // We use 32bit addresses so set this to 0
    WriteOpReg(EHCI_OPS_CtrlDSSegment, 0);

    // Set Interrupt Enable register for following interrupts:
    // Short Packet, Completion of frame, Error with transaction and port change interrupt
    WriteOpReg(EHCI_OPS_USBInterrupt, 0x07);

    // Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
bool EHCIController::ResetController()
{
    Log(Info, "[EHCI] Controller reset......");

    // First halt the controller, and give it some time to do so
	WriteOpReg(EHCI_OPS_USBCommand, 0);
	System::pit->Sleep(USB_TRSTRCY);

	// Then send the reset
	WriteOpReg(EHCI_OPS_USBCommand, (1<<1));

	uint32_t timeout = 1000;
	while (ReadOpReg(EHCI_OPS_USBCommand) & (1<<1)) {
		System::pit->Sleep(10);
        timeout -= 10;
		if (timeout < 0)
			return false;
	}

	return true;
}
void EHCIController::InitializeAsyncList()
{
    e_queueHead_t* queueHeadPtr = this->asyncList;
    uint32_t queueHeadPhysPtr = this->asyncListPhys;
    // The async queue (Control and Bulk TD's) is a round robin set of 16 Queue Heads.
    for (int i = 0; i < 16; i++) {
        queueHeadPtr->horzPointer = (queueHeadPhysPtr + sizeof(e_queueHead_t)) | QH_HS_TYPE_QH | QH_HS_T0;
        queueHeadPtr->horzPointerVirt = queueHeadPtr + 1;
        queueHeadPtr->prevPointerPhys = queueHeadPhysPtr - sizeof(e_queueHead_t);
        queueHeadPtr->prevPointerVirt = queueHeadPtr - 1;
        queueHeadPtr->flags = (0 << 16) | ((i==0) ? (1<<15) : (0<<15)) | (QH_HS_EPS_HS<<12) | (0<<8) | 0;
        queueHeadPtr->hubFlags = (1<<30);
        queueHeadPtr->transferDescriptor.nextQTD = QH_HS_T1;
        queueHeadPtr->transferDescriptor.altNextQTD = QH_HS_T1;
        queueHeadPtr++;
        queueHeadPhysPtr += sizeof(e_queueHead_t);
    }
    
    // Backup and point the last one at the first one
    queueHeadPtr--;
    queueHeadPtr->horzPointer = (this->asyncListPhys | QH_HS_TYPE_QH | QH_HS_T0);
    queueHeadPtr->horzPointerVirt = this->asyncList;
    
    // Setup previous pointers for first item
    this->asyncList->prevPointerPhys = queueHeadPhysPtr + 15 * sizeof(e_queueHead_t);
    this->asyncList->prevPointerVirt = queueHeadPtr + 15;
}
void EHCIController::InitializePeriodicList()
{
  // The periodic list is a round robin set of (256, 512, or) 1204 list pointers.
  for (int i = 0; i < 1024; i++)
    this->periodicList[i] = QH_HS_TYPE_QH | QH_HS_T1;
}
void EHCIController::Setup()
{
    Log(Info, "[EHCI] Starting controller");

    // Allocate Async and Periodic lists
    this->asyncList = (e_queueHead_t*)KernelHeap::alignedMalloc(16 * sizeof(e_queueHead_t), 4096, &this->asyncListPhys);
    this->periodicList = (uint32_t*)KernelHeap::alignedMalloc(1024 * sizeof(uint32_t), 4096, &this->periodicListPhys);

    Log(Info, "[EHCI] Async List allocated %x (virt) and %x (phys)", this->asyncList, this->asyncListPhys);
    Log(Info, "[EHCI] Periodic List allocated %x (virt) and %x (phys)", this->periodicList, this->periodicListPhys);

    // Clear them out
    MemoryOperations::memset(this->asyncList, 0, 16 * sizeof(e_queueHead_t));
    MemoryOperations::memset(this->periodicList, 0, 1024 * sizeof(uint32_t));

    // And then initialize them
    InitializeAsyncList();
    InitializePeriodicList();

    // Set List Pointers
    WriteOpReg(EHCI_OPS_PeriodicListBase, this->periodicListPhys);
    WriteOpReg(EHCI_OPS_AsyncListBase, this->asyncListPhys);

    // Read port specific events
    bool hasPPC = (readMemReg(this->regBase + EHCI_CAPS_HCCParams) & (1<<18)) != 0;

    // Read configuration of controller and attempt to start it
    uint32_t cmdConfig = ReadOpReg(EHCI_OPS_USBCommand);
    cmdConfig &= ~((0xFF << 16) | (1 << 15)); // Reset special interrupt tresholds and port specific events
    
    // Start controller
    WriteOpReg(EHCI_OPS_USBCommand, cmdConfig | (8<<16) | (hasPPC ? (1<<15) : 0) | (1<<5) | (1<<4) | (1<<0));

    bool running = false;
	for (int i = 0; i < 10; i++) {
		uint32_t status = ReadOpReg(EHCI_OPS_USBStatus);

		if (status & (1 << 12)) {
			System::pit->Sleep(10);
		} else {
			running = true;
			break;
		}
	}

    if(!running) {
        Log(Error, "[EHCI] Controller did not start!");
        System::usbManager->RemoveController(this);
        return;
    }

    // Take ownership of root hub ports
    WriteOpReg(EHCI_OPS_ConfigFlag, 1);
    System::pit->Sleep(100);

    // If we have control to change the port power, we need to power each port to 1
    if (readMemReg(this->regBase + EHCI_CAPS_HCSParams) & (1<<4))
        for (int i = 0; i < numPorts; i++)
            WriteOpReg(EHCI_OPS_PortStatus + (i * 4), ReadOpReg(EHCI_OPS_PortStatus + (i * 4)) | EHCI_PORT_PP);
    
    // After powering a port, we must wait 20mS before using it.
    System::pit->Sleep(20);

    // We should be ready to detect any ports that are occupied
    for (int i = 0; i < numPorts; i++) {
        // Power and reset the port
        if (ResetPort(i)) {
            Log(Info, "[EHCI] Found device at port %d", i);
            SetupNewDevice(i);
        }
    }
}
uint32_t EHCIController::HandleInterrupt(uint32_t esp)
{
    uint32_t val = ReadOpReg(EHCI_OPS_USBStatus);
    //Log(Info, "EHCI Interrupt, Status = %x", val);

    if(val == 0) // Interrupt came from another EHCI device
    {
        //Log(Warning, "Interrupt came from another EHCI device!\n");
        return esp;
    }

    WriteOpReg(EHCI_OPS_USBStatus, val);

    if (val & (1<<1))
    {
        Log(Error, "[EHCI] USB Error Interrupt");
    }

    if (val & (1<<2))
    {
        Log(Info, "[EHCI] Port Change");
    }

    if (val & (1<<3))
    {
        //Log(Info, "EHCI: Frame List Rollover Interrupt");
    }

    if (val & (1<<4))
    {
        Log(Error, "[EHCI] Host System Error");
    }

    return esp;
}
void EHCIController::ControllerChecksThread()
{
    for (int i = 0; i < numPorts; i++)
    {
        uint32_t HCPortStatusOff = EHCI_OPS_PortStatus + (i * 4);
        uint32_t portStatus = ReadOpReg(HCPortStatusOff);
        if (portStatus & EHCI_PORT_CSC)
        {
            Log(Info, "[EHCI] Port %d Connection change, now %s", i, (portStatus & EHCI_PORT_CCS) ? "Connected" : "Not Connected");
            portStatus |= EHCI_PORT_CSC; // Clear bit
            WriteOpReg(HCPortStatusOff, portStatus);

            if (portStatus & EHCI_PORT_CCS) { // Connected
                if(ResetPort(i))
                    SetupNewDevice(i);
            }
            else {
                System::usbManager->RemoveDevice(this, i);          
            }
        }
    }
}
bool EHCIController::ResetPort(uint8_t port)
{
    uint32_t HCPortStatusOff = EHCI_OPS_PortStatus + (port * 4);
    
    // Clear the enable bit and status change bits (making sure the PP is set)
    WriteOpReg(HCPortStatusOff, EHCI_PORT_PP | EHCI_PORT_OVER_CUR_C | EHCI_PORT_ENABLE_C | EHCI_PORT_CSC);
    
    // Read the port and see if a device is attached
    // If device attached and is a hs device, the controller will set the enable bit.
    // If the enable bit is not set, then there was an error or it is a low- or full-speed device.
    // If bits 11:10 = 01b, then it isn't a high speed device anyway, skip the reset.
    uint32_t portValue = ReadOpReg(HCPortStatusOff);
    if ((portValue & EHCI_PORT_CCS) && (((portValue & EHCI_PORT_LINE_STATUS) >> 10) != 0x01)) {
        // Set bit 8 (writing a zero to bit 2)
        WriteOpReg(HCPortStatusOff, EHCI_PORT_PP | EHCI_PORT_RESET);
        System::pit->Sleep(USB_TDRSTR);  // At least 50 ms for a root hub
        
        // Clear the reset bit leaving the power bit set
        WriteOpReg(HCPortStatusOff, EHCI_PORT_PP);
        System::pit->Sleep(USB_TRSTRCY);
    }
    
    portValue = ReadOpReg(HCPortStatusOff);
    if (portValue & EHCI_PORT_CCS) {
        // If after the reset, the enable bit is set, we have a high-speed device
        if (portValue & EHCI_PORT_ENABLED) {
            // Found a high-speed device.
            // Clear the status change bit(s)
            WriteOpReg(HCPortStatusOff, ReadOpReg(HCPortStatusOff) & EHCI_PORT_WRITE_MASK);

            Log(Info, "[EHCI] Found High-Speed Device at port %d", port);
            return true;
        } else {
            Log(Info, "[EHCI] Found a low- or full-speed device. Releasing Control.");

            // Disable and power off the port
            WriteOpReg(HCPortStatusOff, 0);
            System::pit->Sleep(10);
            
            // Release ownership of the port.
            WriteOpReg(HCPortStatusOff, EHCI_PORT_OWNER);

            // Wait for the owner bit to actually be set, and the ccs bit to clear
            if(!WaitForRegister(HCPortStatusOff, (EHCI_PORT_OWNER | EHCI_PORT_CCS), EHCI_PORT_OWNER, 250)) {
                Log(Error, "[EHCI] Could not release control of device.");
                return false;
            }
        }
    }
    return false;
}
// This routine waits for the value read at (base, reg) and'ed by mask to equal result.
// It returns true if this happens before the alloted time expires
// returns false if this does not happen
bool EHCIController::WaitForRegister(const uint32_t reg, const uint32_t mask, const uint32_t result, unsigned ms) {
    do {
        if ((ReadOpReg(reg) & mask) == result)
            return true;
        
        System::pit->Sleep(1);
    } while (--ms);
    
    return false;
}
bool EHCIController::SetupNewDevice(const int port) 
{
    struct DEVICE_DESC dev_desc;
    
    // Send the "get_descriptor" packet (get 18 bytes)
    if (!ControlIn(&dev_desc, 0, EHCI_MPS, 18, STDRD_GET_REQUEST, GET_DESCRIPTOR, DEVICE))
        return false;
    
    // Reset the port
    ResetPort(port);
    
    // Set address
    ControlOut(0, EHCI_MPS, 0, STDRD_SET_REQUEST, SET_ADDRESS, 0, this->newDeviceAddress);

    // Setup Device
    USBDevice* newDev = new USBDevice();
    newDev->controller = this;
    newDev->devAddress = this->newDeviceAddress++;
    newDev->portNum = port;
                
    System::usbManager->AddDevice(newDev);
    
    return true;
}

bool EHCIController::StopLegacy(const uint32_t params)
{
    const uint8_t eecp = (uint8_t) ((params & 0x0000FF00) >> 8);
    
    if (eecp >= 0x40) 
    {
        // set bit 24 asking the BIOS to release ownership
        System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHCI_LEGACY_USBLEGSUP, 
        (System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHCI_LEGACY_USBLEGSUP) | EHCI_LEGACY_OS_OWNED));
        
        // Timeout if bit 24 is not set and bit 16 is not clear after EHC_LEGACY_TIMEOUT milliseconds
        int timeout = EHCI_LEGACY_TIMEOUT;
        while (timeout--) {
            if ((System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHCI_LEGACY_USBLEGSUP) & EHCI_LEGACY_OWNED_MASK) == EHCI_LEGACY_OS_OWNED)
                return true;
            System::pit->Sleep(1);
        }
        
        return false;
    } else
        return true;
}

void EHCIController::MakeSetupTransferDesc(e_transferDescriptor_t* tdVirt, const uint32_t tdPhys, uint32_t bufPhys)
{    
    tdVirt->nextQTD = tdPhys + sizeof(e_transferDescriptor_t);
    tdVirt->nextQTDVirt = tdVirt + 1;
    tdVirt->altNextQTD = QH_HS_T1;
    tdVirt->altNextQTDVirt = 0;
    tdVirt->flags = (0<<31) | (8<<16) | (0<<15) | (0<<12) | (3<<10) | (EHCI_TD_PID_SETUP<<8) | 0x80;
    
    tdVirt->bufPtr[0] = bufPhys;

    bufPhys = (bufPhys + 0x1000) & ~0x0FFF;
    tdVirt->bufPtr[1] = bufPhys;
    tdVirt->bufPtr[2] = bufPhys + 0x1000;
    tdVirt->bufPtr[3] = bufPhys + 0x2000;
    tdVirt->bufPtr[4] = bufPhys + 0x3000;
}

void EHCIController::MakeTransferDesc(e_transferDescriptor_t* currentTD, uint32_t physAddr, e_transferDescriptor_t* status_qtd, const uint32_t status_qtdPhys, uint32_t bufferPhys, int size, const bool last, 
                uint8_t data0, const uint8_t dir, const uint16_t mps) 
{   
    do {        
        currentTD->nextQTD = (last && (size <= mps)) ? QH_HS_T1 : ((physAddr + sizeof(e_transferDescriptor_t)) | QH_HS_T0);
        currentTD->nextQTDVirt = (last && (size <= mps)) ? 0 : (currentTD + 1);
        currentTD->altNextQTD = (!status_qtdPhys) ? QH_HS_T1 : status_qtdPhys;
        currentTD->altNextQTDVirt = (!status_qtd) ? 0 : status_qtd;
        currentTD->flags = (data0<<31) | (((size < mps) ? size : mps)<<16) | (0<<15) | (0<<12) | (3<<10) | (dir<<8) | 0x80;
        currentTD->bufPtr[0] = bufferPhys;
        if (bufferPhys) {
            uint32_t buff = (bufferPhys + 0x1000) & ~0x0FFF;
            currentTD->bufPtr[1] = buff;
            currentTD->bufPtr[2] = buff + 0x1000;
            currentTD->bufPtr[3] = buff + 0x2000;
            currentTD->bufPtr[4] = buff + 0x3000;
        }
        
        data0 ^= 1;
        currentTD++;
        physAddr += sizeof(e_transferDescriptor_t);

        size -= mps;
        bufferPhys += mps;
    } while (size > 0);
}

void EHCIController::InsertIntoQueue(e_queueHead_t* item, uint32_t itemPhys, const uint8_t type) {
    item->horzPointer = this->asyncList->horzPointer;
    item->horzPointerVirt = this->asyncList->horzPointerVirt;

    this->asyncList->horzPointer = itemPhys | type;
    this->asyncList->horzPointerVirt = item;

    item->prevPointerPhys = this->asyncListPhys;
    item->prevPointerVirt = this->asyncList;
}

// removes a queue from the async list
// EHCI section 4.8.2, shows that we must watch for three bits before we have "fully and successfully" removed
// the queue(s) from the list
bool EHCIController::RemoveFromQueue(e_queueHead_t* item) 
{  
    e_queueHead_t* prevQH = item->prevPointerVirt;
    e_queueHead_t* nextQH = item->horzPointerVirt;
    
    prevQH->horzPointer = item->horzPointer;
    prevQH->horzPointerVirt = nextQH;

    nextQH->prevPointerPhys = item->prevPointerPhys;
    nextQH->prevPointerVirt = item->prevPointerVirt;
    
    // now wait for the successful "doorbell"
    // set bit 6 in command register (to tell the controller that something has been removed from the schedule)
    // then watch for bit 5 in the status register.  Once it is set, we can assume all removed correctly.
    // We ignore the interrupt on async bit in the USBINTR.  We don't need an interrupt here.
    uint32_t command = ReadOpReg(EHCI_OPS_USBCommand);
    WriteOpReg(EHCI_OPS_USBCommand, command | (1<<6));
    if (WaitForRegister(EHCI_OPS_USBStatus, (1<<5), (1<<5), 100)) {
        WriteOpReg(EHCI_OPS_USBStatus, (1<<5)); // acknowledge the bit
        return true;
    } else
        return false;
}

int EHCIController::WaitForTransferComplete(e_transferDescriptor_t* td, const uint32_t timeout, bool* spd) 
{  
    int ret = -1;
    uint32_t status;
    
    int timer = timeout;
    while (timer) {
        status = td->flags & ~1;  // ignore bit 0 (?)
        if ((status & 0x00000080) == 0) {
            ret = 1;
            if ((status & 0x7F) == 0x00) {
                if ((((status & 0x7FFF0000) >> 16) > 0) && (((status & (3<<8))>>8) == 1))
                    if (spd) 
                        *spd = true;
            } else {
                if (status & (1<<6)) {
                    ret = 0; //ERROR_STALLED;
                    Log(Error, "EHCI qtd->status = ERROR_STALLED");
                }
                else if (status & (1<<5)) {
                    ret = 0; //ERROR_DATA_BUFFER_ERROR;
                    Log(Error, "EHCI qtd->status = ERROR_DATA_BUFFER_ERROR");
                }
                else if (status & (1<<4)) {
                    ret = 0; //ERROR_BABBLE_DETECTED;
                    Log(Error, "EHCI qtd->status = ERROR_BABBLE_DETECTED");
                }
                else if (status & (1<<3)) {
                    ret = 0; //ERROR_NAK;
                    Log(Error, "EHCI qtd->status = ERROR_NAK");
                }
                else if (status & (1<<2)) {
                    ret = 0; //ERROR_TIME_OUT;
                    Log(Error, "EHCI qtd->status = ERROR_TIME_OUT");
                }
                else {
                    Log(Error, "EHCI qtd->status = %x", status);
                    ret = 0; //ERROR_UNKNOWN;
                }
                return ret;
            }
            if ((((status & 0x7FFF0000) >> 16) > 0) && (((status & (3<<8))>>8) == 1)) {
                if ((td->altNextQTD & 1) == 0) {
                    td = (e_transferDescriptor_t*)td->altNextQTDVirt;
                    timer = timeout;
                } 
                else
                    return ret;
            } else {
                if ((td->nextQTD & 1) == 0) {
                    td = (e_transferDescriptor_t*)td->nextQTDVirt;
                    timer = timeout;
                } 
                else
                    return ret;
            }
        }
        System::pit->Sleep(1);
        timer--;
    }
    
    if (ret == -1) {
        Log(Error, "USB EHCI Interrupt wait timed out.");
        ret = 0; //ERROR_TIME_OUT;
    }
    
    return ret;
}

bool EHCIController::ControlOut(const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index)
{
    // Create setupPacket
    uint32_t setupPacketPhys;
    REQUEST_PACKET* setupPacket = (REQUEST_PACKET*)KernelHeap::alignedMalloc(sizeof(REQUEST_PACKET), 16, &setupPacketPhys);
    {
        setupPacket->request_type = requestType;
        setupPacket->request = request;
        setupPacket->value = (valueHigh << 8) | valueLow;
        setupPacket->index = index;
        setupPacket->length = len;
    }

    uint32_t queuePhys; // Physical address of queue
    uint32_t td0Phys; // Physical address of start of transfer descriptors
    e_queueHead_t* queue = (e_queueHead_t*)KernelHeap::alignedMalloc(sizeof(e_queueHead_t), 64, &queuePhys);
    e_transferDescriptor_t* td0 = (e_transferDescriptor_t*)KernelHeap::alignedMalloc(sizeof(e_transferDescriptor_t) * 2, 64, &td0Phys);
    
    // Clear both buffers to 0
    MemoryOperations::memset(queue, 0, sizeof(e_queueHead_t));
    MemoryOperations::memset(td0, 0, sizeof(e_transferDescriptor_t) * 2);
    
    // Setup queue head
    queue->flags = (8<<28) | (EHCI_MPS << 16) | (0<<15) | (1<<14) | (QH_HS_EPS_HS<<12) | (ENDP_CONTROL << 8) | (0<<7) | (devAddress & 0x7F);
    queue->hubFlags = (1<<30) | (0<<23) | (0<<16);
    queue->transferDescriptor.nextQTD = td0Phys;

    MakeSetupTransferDesc(td0, td0Phys, setupPacketPhys);
    MakeTransferDesc(td0+1, td0Phys + sizeof(e_transferDescriptor_t), 0, 0, 0, 0, true, 1, EHCI_TD_PID_IN, packetSize);

    InsertIntoQueue(queue, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0, 2000, 0);
    RemoveFromQueue(queue);

    KernelHeap::allignedFree(queue);
    KernelHeap::allignedFree(td0);
    KernelHeap::allignedFree(setupPacket);
    
    return (ret == 1);
}
bool EHCIController::ControlIn(void* targ, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) 
{
    // Create Request Packet
    uint32_t requestPacketPhys;
    REQUEST_PACKET* requestPacket = (REQUEST_PACKET*)KernelHeap::alignedMalloc(sizeof(REQUEST_PACKET), 16, &requestPacketPhys);
    {
        requestPacket->request_type = requestType;
        requestPacket->request = request;
        requestPacket->value = (valueHigh << 8) | valueLow;
        requestPacket->index = index;
        requestPacket->length = len;
    }
    
    uint32_t queuePhys; // Physical address of queue
    uint32_t td0Phys; // Physical address of start of transfer descriptors
    e_queueHead_t* queue = (e_queueHead_t*)KernelHeap::alignedMalloc(sizeof(e_queueHead_t), 64, &queuePhys);
    e_transferDescriptor_t* td0 = (e_transferDescriptor_t*)KernelHeap::alignedMalloc(sizeof(e_transferDescriptor_t) * 16, 64, &td0Phys);
    
    // Clear both buffers to 0
    MemoryOperations::memset(queue, 0, sizeof(e_queueHead_t));
    MemoryOperations::memset(td0, 0, sizeof(e_transferDescriptor_t) * 16);
    
    uint32_t bufferPhys;
    uint8_t* bufferVirt = (uint8_t*)KernelHeap::malloc(len, &bufferPhys);  // get a physical address buffer and then copy from it later
    
    const int last = 1 + ((len + (packetSize-1)) / packetSize);
    
    // Setup queue head
    queue->flags = (8<<28) | (EHCI_MPS << 16) | (0<<15) | (1<<14) | (QH_HS_EPS_HS<<12) | (ENDP_CONTROL << 8) | (0<<7) | (devAddress & 0x7F);
    queue->hubFlags = (1<<30) | (0<<23) | (0<<16);
    queue->transferDescriptor.nextQTD = td0Phys;

    MakeSetupTransferDesc(td0, td0Phys, requestPacketPhys);
    MakeTransferDesc(td0 + 1, td0Phys + sizeof(e_transferDescriptor_t), td0 + last, td0Phys + (last * sizeof(e_transferDescriptor_t)), bufferPhys, len, false, 1, EHCI_TD_PID_IN, packetSize);
    MakeTransferDesc(td0 + last, td0Phys + (last * sizeof(e_transferDescriptor_t)), 0, 0, 0, 0, true, 1, EHCI_TD_PID_OUT, packetSize);

    InsertIntoQueue(queue, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0, 2000, 0);
    RemoveFromQueue(queue);

    KernelHeap::allignedFree(queue);
    KernelHeap::allignedFree(td0);
    
    if (ret == 1) {
        // now copy from the physical buffer to the specified buffer
        MemoryOperations::memcpy(targ, bufferVirt, len);
        KernelHeap::free(bufferVirt);
        KernelHeap::allignedFree(requestPacket);
        return true;
    }
    
    KernelHeap::free(bufferVirt);
    KernelHeap::allignedFree(requestPacket);
    return false;
}

bool EHCIController::BulkOut(USBEndpoint* toggleSrc, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    uint32_t bufPhys;
    void* tempBuffer = KernelHeap::malloc(len, &bufPhys);
    MemoryOperations::memcpy(tempBuffer, bufPtr, len);

    uint32_t queuePhys; // Physical address of queue
    uint32_t td0Phys; // Physical address of start of transfer descriptors
    e_queueHead_t* queue = (e_queueHead_t*)KernelHeap::alignedMalloc(sizeof(e_queueHead_t), 64, &queuePhys);
    e_transferDescriptor_t* td0 = (e_transferDescriptor_t*)KernelHeap::alignedMalloc(sizeof(e_transferDescriptor_t) * 16, 64, &td0Phys);
    
    // Clear both buffers to 0
    MemoryOperations::memset(queue, 0, sizeof(e_queueHead_t));
    MemoryOperations::memset(td0, 0, sizeof(e_transferDescriptor_t) * 16);
    
    // Setup queue head
    queue->flags = (8<<28) | (packetSize << 16) | (0<<15) | (1<<14) | (QH_HS_EPS_HS<<12) | (endP << 8) | (0<<7) | (devAddress & 0x7F);
    queue->hubFlags = (1<<30) | (0<<23) | (0<<16);
    queue->transferDescriptor.nextQTD = td0Phys;

    MakeTransferDesc(td0, td0Phys, 0, 0, bufPhys, len, true, toggleSrc->Toggle(), EHCI_TD_PID_OUT, packetSize);
    for(int t = 0; t < (1 + ((len + (packetSize-1)) / packetSize)); t++)
        toggleSrc->Toggle();

    InsertIntoQueue(queue, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0, 2000, 0);
    RemoveFromQueue(queue);

    KernelHeap::allignedFree(queue);
    KernelHeap::allignedFree(td0);
    KernelHeap::free(tempBuffer);
    
    return (ret == 1);
}
bool EHCIController::BulkIn(USBEndpoint* toggleSrc, const int devAddress, const int packetSize, const int endP, void* bufPtr, const int len)
{
    uint32_t bufferPhys;
    uint8_t* bufferVirt = (uint8_t*)KernelHeap::malloc(len, &bufferPhys);  // get a physical address buffer and then copy from it later

    uint32_t queuePhys; // Physical address of queue
    uint32_t td0Phys; // Physical address of start of transfer descriptors
    e_queueHead_t* queue = (e_queueHead_t*)KernelHeap::alignedMalloc(sizeof(e_queueHead_t), 64, &queuePhys);
    e_transferDescriptor_t* td0 = (e_transferDescriptor_t*)KernelHeap::alignedMalloc(sizeof(e_transferDescriptor_t) * 16, 64, &td0Phys);
    
    // Clear both buffers to 0
    MemoryOperations::memset(queue, 0, sizeof(e_queueHead_t));
    MemoryOperations::memset(td0, 0, sizeof(e_transferDescriptor_t) * 16);
    
    // Setup queue head
    queue->flags = (8<<28) | (packetSize << 16) | (0<<15) | (1<<14) | (QH_HS_EPS_HS<<12) | (endP << 8) | (0<<7) | (devAddress & 0x7F);
    queue->hubFlags = (1<<30) | (0<<23) | (0<<16);
    queue->transferDescriptor.nextQTD = td0Phys;

    MakeTransferDesc(td0, td0Phys, 0, 0, bufferPhys, len, true, toggleSrc->Toggle(), EHCI_TD_PID_IN, packetSize);
    for(int t = 0; t < (1 + ((len + (packetSize-1)) / packetSize)); t++)
        toggleSrc->Toggle();

    InsertIntoQueue(queue, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0, 2000, 0);
    RemoveFromQueue(queue);
    
    if (ret == 1) {
        // now copy from the physical buffer to the specified buffer
        MemoryOperations::memcpy(bufPtr, bufferVirt, len);
    }
    
    KernelHeap::allignedFree(queue);
    KernelHeap::allignedFree(td0);
    KernelHeap::free(bufferVirt);
    
    return (ret == 1);
}

uint32_t EHCIController::ReadOpReg(uint32_t reg)
{
    return readMemReg(regBase + operRegsOffset + reg);
}

void EHCIController::WriteOpReg(uint32_t reg, uint32_t val)
{
    writeMemReg(regBase + operRegsOffset + reg, val);
}

void EHCIController::DisplayRegisters()
{
    Log(Info, "------------------- EHCI Register Dump-----------------------");
    Log(Info, "EHCI %s %x", "EHCI_CAPS_CapLength",      readMemReg(this->regBase + EHCI_CAPS_CapLength));
    Log(Info, "EHCI %s %x", "EHCI_CAPS_Reserved",       readMemReg(this->regBase + EHCI_CAPS_Reserved));
    Log(Info, "EHCI %s %x", "EHCI_CAPS_IVersion",       readMemReg(this->regBase + EHCI_CAPS_IVersion));
    Log(Info, "EHCI %s %x", "EHCI_CAPS_HCSParams",      readMemReg(this->regBase + EHCI_CAPS_HCSParams));
    Log(Info, "EHCI %s %x", "EHCI_CAPS_HCCParams",      readMemReg(this->regBase + EHCI_CAPS_HCCParams));
    Log(Info, "EHCI %s %x", "EHCI_CAPS_HCSPPortRoute",  readMemReg(this->regBase + EHCI_CAPS_HCSPPortRoute));

    Log(Info, "EHCI %s %x", "EHCI_OPS_USBCommand",          ReadOpReg(EHCI_OPS_USBCommand));
    Log(Info, "EHCI %s %x", "EHCI_OPS_USBStatus",           ReadOpReg(EHCI_OPS_USBStatus));
    Log(Info, "EHCI %s %x", "EHCI_OPS_USBInterrupt",        ReadOpReg(EHCI_OPS_USBInterrupt));
    Log(Info, "EHCI %s %x", "EHCI_OPS_FrameIndex",          ReadOpReg(EHCI_OPS_FrameIndex));
    Log(Info, "EHCI %s %x", "EHCI_OPS_CtrlDSSegment",       ReadOpReg(EHCI_OPS_CtrlDSSegment));
    Log(Info, "EHCI %s %x", "EHCI_OPS_PeriodicListBase",    ReadOpReg(EHCI_OPS_PeriodicListBase));
    Log(Info, "EHCI %s %x", "EHCI_OPS_AsyncListBase",       ReadOpReg(EHCI_OPS_AsyncListBase));
    Log(Info, "EHCI %s %x", "EHCI_OPS_ConfigFlag",          ReadOpReg(EHCI_OPS_ConfigFlag));
    Log(Info, "EHCI Port dump:");
    for(uint8_t i = 0; i < this->numPorts; i++)
        Log(Info, "     %d. %x", i,          ReadOpReg(EHCI_OPS_PortStatus + (i*4)));
    Log(Info, "-------------------------------------------------------------");
}
void EHCIController::PrintTransferDescriptors(e_transferDescriptor_t* td)
{
    Log(Info, "------- EHCI TD Dump %x -------", (uint32_t)td);
    
    while(td)
    {
        Log(Info, "%x: NextTD -> %x : %x", (uint32_t)td, td->nextQTD, (uint32_t)td->nextQTDVirt);
        Log(Info, "%x: AltTD  -> %x : %x", (uint32_t)td, td->altNextQTD, (uint32_t)td->altNextQTDVirt);
        Log(Info, "%x: Flags  -> %x", (uint32_t)td, td->flags);
        Log(Info, "  |-> Status=%x PID=%d C-ERR=%d C-PAGE=%d IOC=%d TBTT=%d DT=%d", td->flags & 0xFF, (td->flags & (0b11<<8))>>8, (td->flags & (0b11<<10))>>10, (td->flags & (0b111<<12))>>12, (td->flags & (1<<15))>>15, (td->flags & (0b111111111111111<<16))>>16, (td->flags & (1<<31))>>31);
        Log(Info, "------------------------------");

        td = td->nextQTDVirt;
    }
}
void EHCIController::PrintQueueHead(e_queueHead_t* qh)
{
    Log(Info, "------- EHCI QH Dump %x -------", (uint32_t)qh);
    
    Log(Info, "%x: Horizontal -> %x : %x", (uint32_t)qh, qh->horzPointer, (uint32_t)qh->horzPointerVirt);
    Log(Info, "%x: Previous   -> %x : %x", (uint32_t)qh, qh->prevPointerPhys, (uint32_t)qh->prevPointerVirt);
    Log(Info, "%x: Flags      -> %x", (uint32_t)qh, qh->flags);
    Log(Info, "  |-> Addr=%d I=%d Endp=%d EPS=%d dtc=%d H=%d MaxPacket=%d C=%d RL=%d", qh->flags & 0x7F, (qh->flags & (1<<7))>>7, (qh->flags & (0b111<<8))>>8, (qh->flags & (0b11<<12))>>12, (qh->flags & (1<<14))>>14, (qh->flags & (1<<15))>>15, (qh->flags & (0b1111111111<<16))>>16, (qh->flags & (1<<27))>>27, (qh->flags & (0b111<<28))>>28);
    Log(Info, "%x: Hubflags   -> %x", (uint32_t)qh, qh->hubFlags);
    Log(Info, "  |-> sMask=%x cMask=%x HubAddr=%d Port=%d Mult=%d", qh->hubFlags & 0xFF, (qh->hubFlags & (0xFF<<8))>>8, (qh->hubFlags & (0b11111<<16))>>16, (qh->hubFlags & (0b1111111<<23))>>23, (qh->hubFlags & (0b11<<30))>>30);
    Log(Info, "%x: CurrentTD  -> %x", (uint32_t)qh, qh->curQTD);

    e_transferDescriptor_t* td = &qh->transferDescriptor;

    Log(Info, "------ EHCI TD Overlay of QH --");
    Log(Info, "    NextTD -> %x : %x", td->nextQTD, (uint32_t)td->nextQTDVirt);
    Log(Info, "    AltTD  -> %x : %x", td->altNextQTD, (uint32_t)td->altNextQTDVirt);
    Log(Info, "    Flags  -> %x", td->flags);
    Log(Info, "       |-> Status=%x PID=%d C-ERR=%d C-PAGE=%d IOC=%d TBTT=%d DT=%d", td->flags & 0xFF, (td->flags & (0b11<<8))>>8, (td->flags & (0b11<<10))>>10, (td->flags & (0b111<<12))>>12, (td->flags & (1<<15))>>15, (td->flags & (0b111111111111111<<16))>>16, (td->flags & (1<<31))>>31);

    Log(Info, "------------------------------");
}
void EHCIController::PrintAsyncQueue()
{
    Log(Info, "------------------------------");
    Log(Info, "     Start Of Queue Dump      ");
    Log(Info, "------------------------------");

    e_queueHead_t* qh = this->asyncList;
    while(qh)
    {
        PrintQueueHead(qh);

        qh = qh->horzPointerVirt;
        if(qh == this->asyncList)
            break;
    }

    Log(Info, "------------------------------");
    Log(Info, "     End Of Queue Dump      ");
    Log(Info, "------------------------------");
}

/////////
// USB Controller Functions
/////////

bool EHCIController::BulkIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    return BulkIn(device->endpoints[endP-1], device->devAddress, device->endpoints[endP-1]->maxPacketSize, endP, retBuffer, len);
}
bool EHCIController::BulkOut(USBDevice* device, void* sendBuffer, int len, int endP)
{
    return BulkOut(device->endpoints[endP-1], device->devAddress, device->endpoints[endP-1]->maxPacketSize, endP, sendBuffer, len);
}

bool EHCIController::ControlIn(USBDevice* device, void* target, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index)
{
    return ControlIn(target, device->devAddress, 64, len, requestType, request, valueHigh, valueLow, index);
}
bool EHCIController::ControlOut(USBDevice* device, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index)
{
    return ControlOut(device->devAddress, 64, len, requestType, request, valueHigh, valueLow, index);
}

void EHCIController::InterruptIn(USBDevice* device, int len, int endP)
{

}