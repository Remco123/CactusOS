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
    Log(Info, "EHCI %s %x", "EHCI_OPS_CtrlDSSegemnt",       ReadOpReg(EHCI_OPS_CtrlDSSegemnt));
    Log(Info, "EHCI %s %x", "EHCI_OPS_PeriodicListBase",    ReadOpReg(EHCI_OPS_PeriodicListBase));
    Log(Info, "EHCI %s %x", "EHCI_OPS_AsyncListBase",       ReadOpReg(EHCI_OPS_AsyncListBase));
    Log(Info, "EHCI %s %x", "EHCI_OPS_ConfigFlag",          ReadOpReg(EHCI_OPS_ConfigFlag));
    Log(Info, "EHCI %s %x", "EHCI_OPS_PortStatus",          ReadOpReg(EHCI_OPS_PortStatus));
    Log(Info, "-------------------------------------------------------------");
}

bool EHCIController::Initialize()
{
    BaseAddressRegister BAR0 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 0);
    if(BAR0.type == InputOutput)
        return false; // We only want memory mapped controllers

    uint32_t memStart = pageRoundDown((uint32_t)BAR0.address); // Assuming 32-Bit address
    uint32_t memEnd = pageRoundUp((uint32_t)BAR0.address + BAR0.size);
    
    // Allocate virtual chuck of memory that we can use for device
    this->regBase = DeviceHeap::AllocateChunck(memEnd - memStart) + ((uint32_t)BAR0.address % PAGE_SIZE);

    // Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)memStart, (void*)this->regBase, memEnd - memStart, true, true);

    Log(Info, "EHCI Controller Memory addres %x -> Memory Mapped: Start=%x End=%x Size=%x", (uint32_t)BAR0.address, memStart, memEnd, memEnd - memStart);
    Log(Info, "EHCI regBase -> %x", this->regBase);

    // Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, 0x0006);

    // Calculate the operational base
    // Must be before any ReadOpReg() or WriteOpReg() calls
    operRegsOffset = (uint8_t)readMemReg(this->regBase + EHCI_CAPS_CapLength);

    DisplayRegisters();

    // Make sure the run/stop bit is clear
    WriteOpReg(EHCI_OPS_USBCommand, ReadOpReg(EHCI_OPS_USBCommand) & ~(1<<0));

    // Wait for HCHalted bit to be set
    while(!(ReadOpReg(EHCI_OPS_USBStatus & (1<<12))))
        System::pit->Sleep(1);

    // Reset the controller, returning false after 500 ms if it doesn't reset
    int timeout = 500;
    WriteOpReg(EHCI_OPS_USBCommand, ReadOpReg(EHCI_OPS_USBCommand) | (1<<1));
    
    // Give controller some time to initalize the reset
    System::pit->Sleep(10);

    while (ReadOpReg(EHCI_OPS_USBCommand) & (1<<1)) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            return false;
    }

    // Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void EHCIController::InitializeAsyncList()
{
    e_queueHead_t* queueHeadPtr = this->asyncList;
    uint32_t queueHeadPhysPtr = this->asyncListPhys;
    // The async queue (Control and Bulk TD's) is a round robin set of 16 Queue Heads.
    for (int i = 0; i < 16; i++) {
        queueHeadPtr->horzPointer = (queueHeadPhysPtr + sizeof(e_queueHead_t)) | QH_HS_TYPE_QH | QH_HS_T0;
        queueHeadPtr->horzPointerVirt = ((uint32_t)queueHeadPtr + sizeof(e_queueHead_t)) | QH_HS_TYPE_QH | QH_HS_T0;
        queueHeadPtr->flags = (0 << 16) | ((i==0) ? (1<<15) : (0<<15)) | QH_HS_EPS_HS | (0<<8) | 0;
        queueHeadPtr->hubFlags = (1<<30);
        queueHeadPtr->transferDescriptor.nextQTD = QH_HS_T1;
        queueHeadPtr->transferDescriptor.altNextQTD = QH_HS_T1;
        queueHeadPtr++;
        queueHeadPhysPtr += sizeof(e_queueHead_t);
    }
    
    // Backup and point the last one at the first one
    queueHeadPtr--;;
    queueHeadPtr->horzPointer = (this->asyncListPhys | QH_HS_TYPE_QH | QH_HS_T0);
    queueHeadPtr->horzPointerVirt = ((uint32_t)this->asyncList | QH_HS_TYPE_QH | QH_HS_T0);
}
void EHCIController::InitializePeriodicList()
{
  // The periodic list is a round robin set of (256, 512, or) 1204 list pointers.
  for (int i = 0; i < 1024; i++)
    this->periodicList[i] = QH_HS_TYPE_QH | QH_HS_T1;
}
bool EHCIController::EnableList(const bool enable, const uint8_t bit)
{    
    // First make sure that both bits are the same
    // Should not modify the enable bit unless the status bit has the same value
    uint32_t command = ReadOpReg(EHCI_OPS_USBCommand);
    if (WaitForRegister(EHCI_OPS_USBStatus, (1<<(bit + 10)), (command & (1<<(bit + 0))) ? (1<<(bit + 10)) : 0, 100)) {
        if (enable) {
            if (!(command & (1<<(bit + 0))))
                WriteOpReg(EHCI_OPS_USBCommand, command | (1<<(bit + 0)));
            return WaitForRegister(EHCI_OPS_USBStatus, (1<<(bit + 10)), (1<<(bit + 10)), 100);
        } else {
            if (command & (1<<(bit + 0)))
                WriteOpReg(EHCI_OPS_USBCommand, command & ~(1<<(bit + 0)));
            return WaitForRegister(EHCI_OPS_USBStatus, (1<<(bit + 10)), 0, 100);
        }
    }
    
    return false;
}
void EHCIController::Setup()
{
    uint32_t hcsparams = readMemReg(this->regBase + EHCI_CAPS_HCSParams);
    uint32_t hccparams = readMemReg(this->regBase + EHCI_CAPS_HCCParams);

    // Get num_ports from EHCI's HCSPARAMS register
    numPorts = (uint8_t)(hcsparams & 0x0F);  // At least 1 and no more than 15
    Log(Info, "EHCI Found %d root hub ports.", numPorts);

    // Turn off legacy support for Keyboard and Mice
    if (!StopLegacy(hccparams)) {
        Log(Error, "EHCI BIOS did not release Legacy support...");
        System::usbManager->RemoveController(this);
        return;
    }

    // Allocate Async and Periodic lists
    this->asyncList = (e_queueHead_t*)KernelHeap::allignedMalloc(16 * sizeof(e_queueHead_t), 32, &this->asyncListPhys);
    this->periodicList = (uint32_t*)KernelHeap::allignedMalloc(1024 * sizeof(uint32_t), 4096, &this->periodicListPhys);

    // And then initialize them
    InitializeAsyncList();
    InitializePeriodicList();

    // Set List Pointers
    WriteOpReg(EHCI_OPS_PeriodicListBase, this->periodicListPhys);
    WriteOpReg(EHCI_OPS_AsyncListBase, this->asyncListPhys);

    // Set Interrupt Enable register for following interrupts:
    // Short Packet, Completion of frame, Error with transaction and port change interrupt
    WriteOpReg(EHCI_OPS_USBInterrupt, 0x07);

    // Set frame number index to 0
    WriteOpReg(EHCI_OPS_FrameIndex, 0);

    // We use 32bit addresses so set this to 0
    WriteOpReg(EHCI_OPS_CtrlDSSegemnt, 0);

    // Clear status register
    WriteOpReg(EHCI_OPS_USBStatus, 0x3F);

    // Start controller, 8 micro-frames, (frame list size = 1024)
    WriteOpReg(EHCI_OPS_USBCommand, (0x8 << 16) | (1<<0));

    // Enable the asynchronous list
    if (!EnableList(true, 5)) {
        Log(Error, "EHCI Did not enable the Ascynchronous List");
        System::usbManager->RemoveController(this);
        return;
    }
    
    // Enable the periodic list
    if (!EnableList(true, 4)) {
        Log(Error, "EHCI Did not enable the Periodic List");
        System::usbManager->RemoveController(this);
        return;
    }

    // Take ownership of root hub ports
    WriteOpReg(EHCI_OPS_ConfigFlag, 1);

    // If we have control to change the port power, we need to power each port to 1
    if (hcsparams & (1<<4))
        for (int i = 0; i < numPorts; i++)
            WriteOpReg(EHCI_OPS_PortStatus + (i * 4), ReadOpReg(EHCI_OPS_PortStatus + (i * 4)) | EHCI_PORT_PP);
    
    // After powering a port, we must wait 20mS before using it.
    System::pit->Sleep(20);

    // We should be ready to detect any ports that are occupied
    for (int i = 0; i < numPorts; i++) {
        // Power and reset the port
        if (ResetPort(i)) {
            Log(Info, "EHCI, Found device at port %d", i);
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
        Log(Error, "USB Error Interrupt");
    }

    if (val & (1<<2))
    {
        Log(Info, "Port Change");
    }

    if (val & (1<<3))
    {
        Log(Info, "Frame List Rollover Interrupt");
    }

    if (val & (1<<4))
    {
        Log(Error, "Host System Error");
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
            Log(Info, "EHCI Port %d Connection change, now %s", i, (portStatus & EHCI_PORT_CCS) ? "Connected" : "Not Connected");
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

            //Log(Info, "EHCI, Found High-Speed Device at port %d", port);
            return true;
        } else {
            //Log(Info, "Found a low- or full-speed device. Releasing Control.");

            // Disable and power off the port
            WriteOpReg(HCPortStatusOff, 0);
            System::pit->Sleep(10);
            
            // Release ownership of the port.
            WriteOpReg(HCPortStatusOff, EHCI_PORT_OWNER);

            // Wait for the owner bit to actually be set, and the ccs bit to clear
            WaitForRegister(HCPortStatusOff, (EHCI_PORT_OWNER | EHCI_PORT_CCS), EHCI_PORT_OWNER, 25);
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
bool EHCIController::SetupNewDevice(const int port) {
  
    struct DEVICE_DESC dev_desc;
    
    //
    // Since most high-speed devices will only work with a max packet size of 64,
    // we don't request the first 8 bytes, then set the address, and request
    // the all 18 bytes like the uhci/ohci controllers.  However, I have included
    // the code below just to show how it could be done.
    //
    
    uint8_t max_packet = 64;
    
    // Send the "get_descriptor" packet (get 18 bytes)
    if (!ControlIn(&dev_desc, 0, max_packet, 18, STDRD_GET_REQUEST, GET_DESCRIPTOR, DEVICE))
        return false;
    
    // Reset the port
    ResetPort(port);
    
    // Set address
    ControlOut(0, max_packet, 0, STDRD_SET_REQUEST, SET_ADDRESS, 0, dev_address);

    // Setup Device
    USBDevice* newDev = new USBDevice();
    newDev->controller = this;
    newDev->devAddress = dev_address;
    newDev->portNum = port;
                
    System::usbManager->AddDevice(newDev);
    dev_address++;
    
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

void EHCIController::SetupQueueHead(e_queueHead_t* head, const uint32_t qtd, uint8_t endpt, const uint16_t mps, const uint8_t address) {    
    // clear it to zeros
    MemoryOperations::memset((void*)head, 0, sizeof(e_queueHead_t));
    
    head->horzPointer = 1;
    head->horzPointerVirt = 1;
    head->flags = (8<<28) | ((mps & 0x7FF) << 16) | (0<<15) | 
        (1<<14) | (2<<12) | ((endpt & 0x0F) << 8) | (0<<7) | (address & 0x7F);
    head->hubFlags = (1<<30) | (0<<23) | (0<<16);
    head->transferDescriptor.nextQTD = qtd;
}

int EHCIController::MakeSetupTransferDesc(e_TransferDescriptor_t* tdVirt, const uint32_t tdPhys, uint32_t bufPhys) {    
    // clear it to zeros
    MemoryOperations::memset((void*)tdVirt, 0, sizeof(e_TransferDescriptor_t));
    
    tdVirt->nextQTD = tdPhys + sizeof(e_TransferDescriptor_t);
    tdVirt->nextQTDVirt = (uint32_t)tdVirt + sizeof(e_TransferDescriptor_t);
    tdVirt->altNextQTD = QH_HS_T1;
    tdVirt->flags = (0<<31) | (8<<16) | (0<<15) | (0<<12) | (3<<10) |
        (EHCI_TD_PID_SETUP<<8) | 0x80;
    
    tdVirt->bufPtr0 = bufPhys;
    bufPhys = (bufPhys + 0x1000) & ~0x0FFF;
    tdVirt->bufPtr1 = bufPhys;
    tdVirt->bufPtr2 = bufPhys + 0x1000;
    tdVirt->bufPtr3 = bufPhys + 0x2000;
    tdVirt->bufPtr4 = bufPhys + 0x3000;
    
    return 1;
}

int EHCIController::MakeTransferDesc(uint32_t virtAddr, uint32_t physAddr, const uint32_t status_qtdVirt, const uint32_t status_qtdPhys, uint32_t bufferPhys, const uint32_t size, const bool last, 
                uint8_t data0, const uint8_t dir, const uint16_t mps) {

    int cnt = 0, i;
    int sz = size;
    int max_size = (0x1000 - (virtAddr & 0x0FFF)) + (4 * 0x1000);
    if (max_size > mps)
        max_size = mps;
    
    e_TransferDescriptor_t* currentVirt = (e_TransferDescriptor_t*)virtAddr;
    uint32_t currentPhys = physAddr;
    
    do {
        // clear it to zeros
        MemoryOperations::memset((void*)currentVirt, 0, sizeof(e_TransferDescriptor_t));
        
        currentVirt->nextQTD = (currentPhys + sizeof(e_TransferDescriptor_t)) | ((last && (sz <= max_size)) ? QH_HS_T1 : 0);
        currentVirt->nextQTDVirt = ((uint32_t)currentVirt + sizeof(e_TransferDescriptor_t)) | ((last && (sz <= max_size)) ? QH_HS_T1 : 0);
        currentVirt->altNextQTD = (!status_qtdPhys) ? QH_HS_T1 : status_qtdPhys;
        currentVirt->altNextQTDVirt = (!status_qtdVirt) ? QH_HS_T1 : status_qtdVirt;
        currentVirt->flags = (data0<<31) | (((sz < max_size) ? sz : max_size)<<16) | (0<<15) | (0<<12) | (3<<10) | (dir<<8) | 0x80;
        currentVirt->bufPtr0 = bufferPhys;
        if (bufferPhys) {
            uint32_t buff = (bufferPhys + 0x1000) & ~0x0FFF;
            currentVirt->bufPtr1 = buff;
            currentVirt->bufPtr2 = buff + 0x1000;
            currentVirt->bufPtr3 = buff + 0x2000;
            currentVirt->bufPtr4 = buff + 0x3000;
        }
        
        bufferPhys += max_size;
        data0 ^= 1;
        currentVirt++;
        currentPhys += sizeof(e_TransferDescriptor_t);
        cnt++;
        sz -= max_size;
    } while (sz > 0);
    
    return cnt;
}

void EHCIController::InsertIntoQueue(e_queueHead_t* item, uint32_t itemPhys, const uint8_t type) {
    item->horzPointer = this->asyncList->horzPointer;
    item->horzPointerVirt = this->asyncList->horzPointerVirt;

    this->asyncList->horzPointer = itemPhys | type;
    this->asyncList->horzPointerVirt = (uint32_t)item | type;

    item->prevPointer = this->asyncListPhys;
    item->prevPointerVirt = (uint32_t)this->asyncList;
}

// removes a queue from the async list
// EHCI section 4.8.2, shows that we must watch for three bits before we have "fully and successfully" removed
// the queue(s) from the list
bool EHCIController::RemoveFromQueue(e_queueHead_t* item) {
    
    uint32_t prevPhys = item->prevPointer;
    e_queueHead_t* prevVirt = (e_queueHead_t*)item->prevPointerVirt;

    prevVirt->horzPointer = item->horzPointer;
    prevVirt->horzPointerVirt = item->horzPointerVirt;

    uint32_t horzPhys = item->horzPointer & ~EHCI_QUEUE_HEAD_PTR_MASK;
    e_queueHead_t* horzVirt = (e_queueHead_t*)(item->horzPointerVirt & ~EHCI_QUEUE_HEAD_PTR_MASK);
    
    horzVirt->prevPointer = item->prevPointer;
    horzVirt->prevPointerVirt = item->prevPointerVirt;
    
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

int EHCIController::WaitForTransferComplete(e_TransferDescriptor_t* td, const uint32_t timeout, bool* spd) 
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
                    td = (e_TransferDescriptor_t*)td->altNextQTDVirt;
                    timer = timeout;
                } 
                else
                    return ret;
            } else {
                if ((td->nextQTD & 1) == 0) {
                    td = (e_TransferDescriptor_t*)td->nextQTDVirt;
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

    //Allocate enough memory to hold the queue and the TD's
    uint32_t queuePhys;
    e_queueHead_t* queueVirt = (e_queueHead_t*)KernelHeap::allignedMalloc(sizeof(e_queueHead_t) + (2 * sizeof(e_TransferDescriptor_t)), 64, &queuePhys);
    e_TransferDescriptor_t* td0Virt = (e_TransferDescriptor_t*)((uint32_t)queueVirt + sizeof(e_queueHead_t));
    uint32_t td0Phys = queuePhys + sizeof(e_queueHead_t);
    
    SetupQueueHead(queueVirt, td0Phys, ENDP_CONTROL, packetSize, devAddress);
    MakeSetupTransferDesc(td0Virt, td0Phys, setupPacketPhys);
    MakeTransferDesc((uint32_t)td0Virt + sizeof(e_TransferDescriptor_t), td0Phys + sizeof(e_TransferDescriptor_t), 0, 0, 0, 0, true, 1, EHCI_TD_PID_IN, packetSize);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0Virt, 2000, 0);
    RemoveFromQueue(queueVirt);

    KernelHeap::allignedFree(queueVirt);
    KernelHeap::allignedFree(setupPacket);
    
    return (ret == 1);
}
bool EHCIController::ControlIn(void* targ, const int devAddress, const int packetSize, const int len, const uint8_t requestType, const uint8_t request, const uint16_t valueHigh, const uint16_t valueLow, const uint16_t index) 
{
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
    
    //Allocate enough memory to hold the queue and the TD's
    uint32_t queuePhys;
    e_queueHead_t* queueVirt = (e_queueHead_t*)KernelHeap::allignedMalloc(sizeof(e_queueHead_t) + (16 * sizeof(e_TransferDescriptor_t)), 64, &queuePhys);
    e_TransferDescriptor_t* td0Virt = (e_TransferDescriptor_t*)((uint32_t)queueVirt + sizeof(e_queueHead_t));
    uint32_t td0Phys = queuePhys + sizeof(e_queueHead_t);
    
    uint32_t bufferPhys;
    uint8_t* bufferVirt = (uint8_t*)KernelHeap::malloc(len, &bufferPhys);  // get a physical address buffer and then copy from it later
    
    const int last = 1 + ((len + (packetSize-1)) / packetSize);
    
    SetupQueueHead(queueVirt, td0Phys, ENDP_CONTROL, packetSize, devAddress);
    MakeSetupTransferDesc(td0Virt, td0Phys, requestPacketPhys);
    MakeTransferDesc((uint32_t)td0Virt + sizeof(e_TransferDescriptor_t), td0Phys + sizeof(e_TransferDescriptor_t), (uint32_t)td0Virt + (last * sizeof(e_TransferDescriptor_t)), td0Phys + (last * sizeof(e_TransferDescriptor_t)), bufferPhys, len, false, 1, EHCI_TD_PID_IN, packetSize);
    MakeTransferDesc((uint32_t)td0Virt + (last * sizeof(e_TransferDescriptor_t)), td0Phys + (last * sizeof(e_TransferDescriptor_t)), 0, 0, 0, 0, true, 1, EHCI_TD_PID_OUT, packetSize);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0Virt, 2000, 0);
    RemoveFromQueue(queueVirt);

    KernelHeap::allignedFree(queueVirt);
    
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

/////////
// USB Controller Functions
/////////

bool EHCIController::BulkIn(USBDevice* device, void* retBuffer, int len, int endP)
{
    uint32_t bufferPhys;
    uint8_t* bufferVirt = (uint8_t*)KernelHeap::malloc(len, &bufferPhys);  // get a physical address buffer and then copy from it later

    //Allocate enough memory to hold the queue and the TD's
    uint32_t queuePhys;
    e_queueHead_t* queueVirt = (e_queueHead_t*)KernelHeap::allignedMalloc(sizeof(e_queueHead_t) + (16 * sizeof(e_TransferDescriptor_t)), 64, &queuePhys);
    e_TransferDescriptor_t* td0Virt = (e_TransferDescriptor_t*)((uint32_t)queueVirt + sizeof(e_queueHead_t));
    uint32_t td0Phys = queuePhys + sizeof(e_queueHead_t);
    
    SetupQueueHead(queueVirt, td0Phys, endP, device->endpoints[endP-1]->maxPacketSize, device->devAddress);
    MakeTransferDesc((uint32_t)td0Virt, td0Phys, 0, 0, bufferPhys, len, true, 0, EHCI_TD_PID_IN, device->endpoints[endP-1]->maxPacketSize);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0Virt, 2000, 0);
    RemoveFromQueue(queueVirt);

    KernelHeap::allignedFree(queueVirt);
    if(ret == 1) {
        MemoryOperations::memcpy(retBuffer, bufferVirt, len);
    }
    
    delete bufferVirt;
    return (ret == 1);
}
bool EHCIController::BulkOut(USBDevice* device, void* sendBuffer, int len, int endP)
{
    //Allocate enough memory to hold the queue and the TD's
    uint32_t queuePhys;
    e_queueHead_t* queueVirt = (e_queueHead_t*)KernelHeap::allignedMalloc(sizeof(e_queueHead_t) + (16 * sizeof(e_TransferDescriptor_t)), 64, &queuePhys);
    e_TransferDescriptor_t* td0Virt = (e_TransferDescriptor_t*)((uint32_t)queueVirt + sizeof(e_queueHead_t));
    uint32_t td0Phys = queuePhys + sizeof(e_queueHead_t);
    
    SetupQueueHead(queueVirt, td0Phys, endP, device->endpoints[endP-1]->maxPacketSize, device->devAddress);
    MakeTransferDesc((uint32_t)td0Virt, td0Phys, 0, 0, (uint32_t)VirtualMemoryManager::virtualToPhysical(sendBuffer), len, true, 0, EHCI_TD_PID_OUT, device->endpoints[endP-1]->maxPacketSize);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForTransferComplete(td0Virt, 2000, 0);
    RemoveFromQueue(queueVirt);

    KernelHeap::allignedFree(queueVirt);
    
    return (ret == 1);
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