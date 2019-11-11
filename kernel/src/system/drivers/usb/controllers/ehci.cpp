#include <system/drivers/usb/controllers/ehci.h>
#include <system/drivers/usb/usbdefs.h>
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
        return false; //We only want memory mapped controllers
    
    //Save Register memory base
    this->regBase = BAR0.address;

    //Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, 0x0006);

    //Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)this->regBase, (void*)this->regBase, pageRoundUp(BAR0.size), true, true);

    //Calculate the operational base
    //Must be before any ReadOpReg() or WriteOpReg() calls
    operRegsOffset = (uint8_t)readMemReg(this->regBase + EHC_CAPS_CapLength);

    //Make sure the run/stop bit is clear
    WriteOpReg(EHC_OPS_USBCommand, ReadOpReg(EHC_OPS_USBCommand) & ~(1<<0));

    //Reset the controller, returning false after 50mS if it doesn't reset
    int timeout = 50;
    WriteOpReg(EHC_OPS_USBCommand, (1<<1));
    while (ReadOpReg(EHC_OPS_USBCommand) & (1<<1)) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            return false;
    }
    /*
    if((readMemReg(this->regBase + EHC_CAPS_HCCParams) & (1<<2)) && (ReadOpReg(EHC_OPS_USBCommand) != 0x0080B00))
        return false;
    if(!(readMemReg(this->regBase + EHC_CAPS_HCCParams) & (1<<2)) && (ReadOpReg(EHC_OPS_USBCommand) != 0x0080000))
        return false;
    */
    //if(ReadOpReg(EHC_OPS_USBStatus) != 0x00001000)
    //    return false;
    if(ReadOpReg(EHC_OPS_USBInterrupt) != 0)
        return false;
    
    if(ReadOpReg(EHC_OPS_FrameIndex) != 0)
        return false;
    if(ReadOpReg(EHC_OPS_CtrlDSSegemnt) != 0)
        return false;
    if(ReadOpReg(EHC_OPS_ConfigFlag) != 0)
        return false;
    
    //Valid Controller if we get here.

    //Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void EHCIController::Setup()
{
    uint32_t hcsparams = readMemReg(this->regBase + EHC_CAPS_HCSParams);
    uint32_t hccparams = readMemReg(this->regBase + EHC_CAPS_HCCParams);

    // Turn off legacy support for Keyboard and Mice
    if (!StopLegacy(hccparams)) {
        Log(Error, "BIOS did not release Legacy support...");
        System::usbManager->RemoveController(this);
        return;
    }

    //Get num_ports from EHCI's HCSPARAMS register
    numPorts = (uint8_t)(hcsparams & 0x0F);  // at least 1 and no more than 15
    Log(Info, "EHCI Found %d root hub ports.", numPorts);

    // allocate then initialize the async queue list (Control and Bulk TD's)
    AsyncListVirt = (uint32_t)KernelHeap::allignedMalloc(16 * EHCI_QUEUE_HEAD_SIZE, 32, &AsyncListPhys);
    InitStackFrame();

    //Set Interrupt Enable register for following interrupts:
    //Short Packet, Completion of frame, Error with transaction and port change interrupt
    WriteOpReg(EHC_OPS_USBInterrupt, 0x07);

    //Set frame number index to 0
    WriteOpReg(EHC_OPS_FrameIndex, 0);

    //Set List Pointers (Implement)
    WriteOpReg(EHC_OPS_PeriodicListBase, 0);
    WriteOpReg(EHC_OPS_AsyncListBase, AsyncListPhys);

    //We use 32bit address so set this to 0
    WriteOpReg(EHC_OPS_CtrlDSSegemnt, 0);

    WriteOpReg(EHC_OPS_USBStatus, 0x3F);
    WriteOpReg(EHC_OPS_USBCommand, 0b10000000000000100001); //0x00080031);

    //Enable the asynchronous list
    if (!EnableAsycnList(true)) {
        Log(Error, "Did not enable the Ascynchronous List");
        System::usbManager->RemoveController(this);
        return;
    }

    //Take ownership of root hub ports
    WriteOpReg(EHC_OPS_ConfigFlag, 1);

    //If we have control to change the port power, we need to power each port to 1
    if (hcsparams & (1<<4))
        for (int i = 0; i < numPorts; i++)
            WriteOpReg(EHC_OPS_PortStatus + (i * 4), ReadOpReg(EHC_OPS_PortStatus + (i * 4)) | EHCI_PORT_PP);
    
    //After powering a port, we must wait 20mS before using it.
    System::pit->Sleep(20);

    //We should be ready to detect any ports that are occupied
    for (int i = 0; i < numPorts; i++) {
        //Power and reset the port
        if (ResetPort(i)) {
            GetDescriptor(i);
        }
    }
}
uint32_t EHCIController::HandleInterrupt(uint32_t esp)
{
    volatile uint32_t val = ReadOpReg(EHC_OPS_USBStatus);
    Log(Info, "EHCI Interrupt, Status = %x", val);

    if(val == 0) // Interrupt came from another EHCI device
    {
        Log(Warning, "Interrupt came from another EHCI device!\n");
        return;
    }

    WriteOpReg(EHC_OPS_USBStatus, val);

    if (val & (1<<1))
    {
        Log(Error, "USB Error Interrupt");
    }

    if (val & (1<<2))
    {
        Log(Info, "Port Change");
        CheckPortChange();
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
void EHCIController::CheckPortChange()
{
    for (int i = 0; i < numPorts; i++)
    {
        uint32_t HCPortStatusOff = EHC_OPS_PortStatus + (i * 4);
        uint32_t portStatus = ReadOpReg(HCPortStatusOff);
        if (portStatus & EHCI_PORT_CSC)
        {
            Log(Info, "Port %d Connection change", i);
            portStatus |= EHCI_PORT_CSC; //Clear bit
            if (portStatus & EHCI_PORT_CCS)
            {
                Log(Info, "Device Attached on port %d", i);
                if(ResetPort(i))
                    GetDescriptor(i);
            }
            else
            {
                Log(Info, "Device detached on port %d", i);
                //TODO: In the future unload driver here if needed                
            }
        }
    }
}
bool EHCIController::ResetPort(uint8_t port)
{
    uint32_t HCPortStatusOff = EHC_OPS_PortStatus + (port * 4);
    uint32_t dword;
    
    //Clear the enable bit and status change bits (making sure the PP is set)
    WriteOpReg(HCPortStatusOff, EHCI_PORT_PP | EHCI_PORT_OVER_CUR_C | EHCI_PORT_ENABLE_C | EHCI_PORT_CSC);
    
    //Read the port and see if a device is attached
    //If device attached and is a hs device, the controller will set the enable bit.
    //If the enable bit is not set, then there was an error or it is a low- or full-speed device.
    //If bits 11:10 = 01b, then it isn't a high speed device anyway, skip the reset.
    dword = ReadOpReg(HCPortStatusOff);
    if ((dword & EHCI_PORT_CCS) && (((dword & EHCI_PORT_LINE_STATUS) >> 10) != 0x01)) {
        //Set bit 8 (writing a zero to bit 2)
        WriteOpReg(HCPortStatusOff, EHCI_PORT_PP | EHCI_PORT_RESET);
        System::pit->Sleep(USB_TDRSTR);  // at least 50 ms for a root hub
        
        //Clear the reset bit leaving the power bit set
        WriteOpReg(HCPortStatusOff, EHCI_PORT_PP);
        System::pit->Sleep(USB_TRSTRCY);
    }
    
    dword = ReadOpReg(HCPortStatusOff);
    if (dword & EHCI_PORT_CCS) {
        //If after the reset, the enable bit is set, we have a high-speed device
        if (dword & EHCI_PORT_ENABLED) {
            //Found a high-speed device.
            //Clear the status change bit(s)
            WriteOpReg(HCPortStatusOff, ReadOpReg(HCPortStatusOff) & EHCI_PORT_WRITE_MASK);

            Log(Info, "EHCI, Found High-Speed Device at port %d", port);
            
            return true;
        } else {
            Log(Info, "Found a low- or full-speed device. Releasing Control.");
            //Disable and power off the port
            WriteOpReg(HCPortStatusOff, 0);
            System::pit->Sleep(10);
            
            //Release ownership of the port.
            WriteOpReg(HCPortStatusOff, EHCI_PORT_OWNER);

            //Wait for the owner bit to actually be set, and the ccs bit to clear
            ehciHandshake(HCPortStatusOff, (EHCI_PORT_OWNER | EHCI_PORT_CCS), EHCI_PORT_OWNER, 25);
        }
    }
    return false;
}
/* This routine waits for the value read at (base, reg) and'ed by mask to equal result.
 * It returns true if this happens before the alloted time expires
 * returns false if this does not happen
 */
bool EHCIController::ehciHandshake(const uint32_t reg, const uint32_t mask, const uint32_t result, unsigned ms) {
    do {
        if ((ReadOpReg(reg) & mask) == result)
            return true;
        
        System::pit->Sleep(1);
    } while (--ms);
    
    return false;
}
bool EHCIController::GetDescriptor(const int port) {
  
    struct DEVICE_DESC dev_desc;
    
    /*
    * Since most high-speed devices will only work with a max packet size of 64,
    *  we don't request the first 8 bytes, then set the address, and request
    *  the all 18 bytes like the uhci/ohci controllers.  However, I have included
    *  the code below just to show how it could be done.
    */
    
    uint8_t max_packet = 64;
    
    // send the "get_descriptor" packet (get 18 bytes)
    if (!ControlIn(&dev_desc, 18, max_packet, 0))
        return false;
    
    // reset the port
    ResetPort(port);
    
    // set address
    SetAddress(max_packet, dev_address);

    //Setup Device
    USBDevice* newDev = new USBDevice();
    newDev->controller = this;
    newDev->devAddress = dev_address;
    newDev->portNum = port;
                
    System::usbManager->AddDevice(newDev);
    dev_address++;
    
    return true;
}

// enable/disable one of the lists.
// if the async member is set, it disables/enables the asynchronous list, else the periodic list
bool EHCIController::EnableAsycnList(const bool enable) {
  
    uint32_t command;
    
    // first make sure that both bits are the same
    // should not modify the enable bit unless the status bit has the same value
    command = ReadOpReg(EHC_OPS_USBCommand);
    if (ehciHandshake(EHC_OPS_USBStatus, (1<<15), (command & (1<<5)) ? (1<<15) : 0, 100)) {
        if (enable) {
            if (!(command & (1<<5)))
                WriteOpReg(EHC_OPS_USBCommand, command | (1<<5));
            
            return ehciHandshake(EHC_OPS_USBStatus, (1<<15), (1<<15), 100);
        } 
        else {
            if (command & (1<<5))
                WriteOpReg(EHC_OPS_USBCommand, command & ~(1<<5));
            
            return ehciHandshake(EHC_OPS_USBStatus, (1<<15), 0, 100);
        }
    }
    
    return false;
}

// initialize the async queue list (Control and Bulk TD's)
void EHCIController::InitStackFrame() {    

    uint32_t queueHeadPtr = AsyncListVirt;
    uint32_t queueHeadPhysPtr = AsyncListPhys;

    // the async queue (Control and Bulk TD's) is a round robin set of 16 Queue Heads.
    for (int i = 0; i < 16; i++) {
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_HORZ_PTR)) = (queueHeadPhysPtr + EHCI_QUEUE_HEAD_SIZE) | QH_HS_TYPE_QH | QH_HS_T0;
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_HORZ_PTR_VIRT)) = (queueHeadPtr + EHCI_QUEUE_HEAD_SIZE) | QH_HS_TYPE_QH | QH_HS_T0;
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_ENDPT_CAPS)) = (0 << 16) | ((i==0) ? (1<<15) : (0<<15)) | QH_HS_EPS_HS | (0<<8) | 0;
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_HUB_INFO)) = (1<<30);
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_NEXT_QTD_PTR)) = QH_HS_T1;
        *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_ALT_NEXT_QTD_PTR)) = QH_HS_T1;
        queueHeadPtr += EHCI_QUEUE_HEAD_SIZE;
        queueHeadPhysPtr += EHCI_QUEUE_HEAD_SIZE;
    }
    
    // backup and point the last one at the first one
    queueHeadPtr -= EHCI_QUEUE_HEAD_SIZE;
    *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_HORZ_PTR)) = (AsyncListPhys | QH_HS_TYPE_QH | QH_HS_T0);
    *((uint32_t*)(queueHeadPtr + EHCI_QH_OFF_HORZ_PTR_VIRT)) = (AsyncListVirt | QH_HS_TYPE_QH | QH_HS_T0);
}

/* Release BIOS ownership of controller
 * On Entry:
 *      pci: pointer to the pci config space we read in
 *   params: the dword value of the capability register
 * On Return:
 *   true if ownership released
 *
 * Set bit 24 to indicate to the BIOS to release ownership
 * The BIOS should clear bit 16 indicating that it has successfully done so
 * Ownership is released when bit 24 is set *and* bit 16 is clear.
 * This will wait EHC_LEGACY_TIMEOUT ms for the BIOS to release ownership.
 *   (It is unknown the exact time limit that the BIOS has to release ownership.)
 */
bool EHCIController::StopLegacy(const uint32_t params) {
    const uint8_t eecp = (uint8_t) ((params & 0x0000FF00) >> 8);
    
    if (eecp >= 0x40) {
        
        // set bit 24 asking the BIOS to release ownership
        System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHC_LEGACY_USBLEGSUP, 
        (System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHC_LEGACY_USBLEGSUP) | EHC_LEGACY_OS_OWNED));
        
        // Timeout if bit 24 is not set and bit 16 is not clear after EHC_LEGACY_TIMEOUT milliseconds
        int timeout = EHC_LEGACY_TIMEOUT;
        while (timeout--) {
            if ((System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, eecp + EHC_LEGACY_USBLEGSUP) & EHC_LEGACY_OWNED_MASK) == EHC_LEGACY_OS_OWNED)
                return true;
            System::pit->Sleep(1);
        }
        
        return false;
    } else
        return true;
}

void EHCIController::SetupQueueHead(uint32_t headVirt, const uint32_t qtd, uint8_t endpt, const uint16_t mps, const uint8_t address) {    
    // clear it to zeros
    MemoryOperations::memset((void*)headVirt, 0, EHCI_QUEUE_HEAD_SIZE);
    
    *((uint32_t*)(headVirt + EHCI_QH_OFF_HORZ_PTR)) = 1;
    *((uint32_t*)(headVirt + EHCI_QH_OFF_HORZ_PTR_VIRT)) = 1;
    *((uint32_t*)(headVirt + EHCI_QH_OFF_ENDPT_CAPS)) = (8<<28) | ((mps & 0x7FF) << 16) | (0<<15) | 
        (1<<14) | (2<<12) | ((endpt & 0x0F) << 8) | (0<<7) | (address & 0x7F);
    *((uint32_t*)(headVirt + EHCI_QH_OFF_HUB_INFO)) = (1<<30) | (0<<23) | (0<<16);
    *((uint32_t*)(headVirt + EHCI_QH_OFF_NEXT_QTD_PTR)) = qtd;
}

int EHCIController::MakeSetupTransferDesc(const uint32_t virtAddr, const uint32_t physAddr, uint32_t bufPhys) {    
    // clear it to zeros
    MemoryOperations::memset((void*)virtAddr, 0, EHCI_TD_SIZE);
    
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_NEXT_TD_PTR)) = physAddr + EHCI_TD_SIZE;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_NEXT_TD_VIRT_PTR)) = virtAddr + EHCI_TD_SIZE; //Custom
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_ALT_NEXT_QTD_PTR)) = QH_HS_T1;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_ALT_TD_VIRT_PTR)) = QH_HS_T1; //Custom
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_STATUS)) = (0<<31) | (8<<16) | (0<<15) | (0<<12) | (3<<10) |
        (EHCI_TD_PID_SETUP<<8) | 0x80;
    
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_BUFF0_PTR)) = bufPhys;
    bufPhys = (bufPhys + 0x1000) & ~0x0FFF;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_BUFF1_PTR)) = bufPhys;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_BUFF2_PTR)) = bufPhys + 0x1000;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_BUFF3_PTR)) = bufPhys + 0x2000;
    *((uint32_t*)(virtAddr + EHCI_TD_OFF_BUFF4_PTR)) = bufPhys + 0x3000;
    
    return 1;
}

int EHCIController::MakeTransferDesc(uint32_t virtAddr, uint32_t physAddr, const uint32_t status_qtd, uint32_t bufferPhys, const uint32_t size, const bool last, 
                uint8_t data0, const uint8_t dir, const uint16_t mps) {

    int cnt = 0, i;
    int sz = size;
    int max_size = (0x1000 - (virtAddr & 0x0FFF)) + (4 * 0x1000);
    if (max_size > mps)
        max_size = mps;
    
    uint32_t currentVirt = virtAddr;
    uint32_t currentPhys = physAddr;
    
    do {
        // clear it to zeros
        MemoryOperations::memset((void*)currentVirt, 0, EHCI_TD_SIZE);
        
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_NEXT_TD_PTR)) = (currentPhys + EHCI_TD_SIZE) | ((last && (sz <= max_size)) ? QH_HS_T1 : 0);
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_NEXT_TD_VIRT_PTR)) = (currentVirt + EHCI_TD_SIZE) | ((last && (sz <= max_size)) ? QH_HS_T1 : 0); //Custom
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_ALT_NEXT_QTD_PTR)) = (!status_qtd) ? QH_HS_T1 : status_qtd;
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_ALT_TD_VIRT_PTR)) = (!status_qtd) ? QH_HS_T1 : status_qtd; //Custom
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_STATUS)) = (data0<<31) | (((sz < max_size) ? sz : max_size)<<16) | (0<<15) | (0<<12) | (3<<10) | (dir<<8) | 0x80;
        *((uint32_t*)(currentVirt + EHCI_TD_OFF_BUFF0_PTR)) = bufferPhys;
        if (bufferPhys) {
            uint32_t buff = (bufferPhys + 0x1000) & ~0x0FFF;
            *((uint32_t*)(currentVirt + EHCI_TD_OFF_BUFF1_PTR)) = buff;
            *((uint32_t*)(currentVirt + EHCI_TD_OFF_BUFF2_PTR)) = buff + 0x1000;
            *((uint32_t*)(currentVirt + EHCI_TD_OFF_BUFF3_PTR)) = buff + 0x2000;
            *((uint32_t*)(currentVirt + EHCI_TD_OFF_BUFF4_PTR)) = buff + 0x3000;
        }
        
        bufferPhys += max_size;
        data0 ^= 1;
        currentVirt += EHCI_TD_SIZE;
        currentPhys += EHCI_TD_SIZE;
        cnt++;
        sz -= max_size;
    } while (sz > 0);
    
    return cnt;
}

void EHCIController::InsertIntoQueue(uint32_t itemVirt, uint32_t itemPhys, const uint8_t type) {
    *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR) = *(uint32_t*)(AsyncListVirt + EHCI_QH_OFF_HORZ_PTR);
    *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR_VIRT) = *(uint32_t*)(AsyncListVirt + EHCI_QH_OFF_HORZ_PTR_VIRT);

    *(uint32_t*)(AsyncListVirt + EHCI_QH_OFF_HORZ_PTR) = itemPhys | type;
    *(uint32_t*)(AsyncListVirt + EHCI_QH_OFF_HORZ_PTR_VIRT) = itemVirt | type;

    *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR) = AsyncListPhys;
    *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR_VIRT) = AsyncListVirt;
}

// removes a queue from the async list
// EHCI section 4.8.2, shows that we must watch for three bits before we have "fully and successfully" removed
//   the queue(s) from the list
bool EHCIController::RemoveFromQueue(uint32_t itemVirt) {
    
    uint32_t prevPhys = *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR);
    uint32_t prevVirt = *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR_VIRT);

    *(uint32_t*)(prevVirt + EHCI_QH_OFF_HORZ_PTR) = *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR);
    *(uint32_t*)(prevVirt + EHCI_QH_OFF_HORZ_PTR_VIRT) = *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR_VIRT);

    uint32_t horzPhys = *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR) & ~EHCI_QUEUE_HEAD_PTR_MASK;
    uint32_t horzVirt = *(uint32_t*)(itemVirt + EHCI_QH_OFF_HORZ_PTR_VIRT) & ~EHCI_QUEUE_HEAD_PTR_MASK;
    
    *(uint32_t*)(horzVirt + EHCI_QH_OFF_PREV_PTR) = *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR);
    *(uint32_t*)(horzVirt + EHCI_QH_OFF_PREV_PTR_VIRT) = *(uint32_t*)(itemVirt + EHCI_QH_OFF_PREV_PTR_VIRT);
    
    // now wait for the successful "doorbell"
    // set bit 6 in command register (to tell the controller that something has been removed from the schedule)
    // then watch for bit 5 in the status register.  Once it is set, we can assume all removed correctly.
    // We ignore the interrupt on async bit in the USBINTR.  We don't need an interrupt here.
    uint32_t command = ReadOpReg(EHC_OPS_USBCommand);
    WriteOpReg(EHC_OPS_USBCommand, command | (1<<6));
    if (ehciHandshake(EHC_OPS_USBStatus, (1<<5), (1<<5), 100)) {
        WriteOpReg(EHC_OPS_USBStatus, (1<<5)); // acknowledge the bit
        return true;
    } else
        return false;
}

int EHCIController::WaitForInterrupt(uint32_t virtAddr, const uint32_t timeout, bool* spd) {
  
    int ret = -1;
    uint32_t status;
    
    int timer = timeout;
    while (timer) {
        status = (*(uint32_t*)(virtAddr + EHCI_TD_OFF_STATUS)) & ~1;  // ignore bit 0 (?)
        if ((status & 0x00000080) == 0) {
            ret = 1;
            if ((status & 0x7F) == 0x00) {
                if ((((status & 0x7FFF0000) >> 16) > 0) && (((status & (3<<8))>>8) == 1))
                    if (spd) 
                        *spd = true;
            } else {
                if (status & (1<<6)) {
                    ret = 0; //ERROR_STALLED;
                    Log(Error, "USB EHCI wait interrupt qtd->status = ERROR_STALLED");
                }
                else if (status & (1<<5)) {
                    ret = 0; //ERROR_DATA_BUFFER_ERROR;
                    Log(Error, "USB EHCI wait interrupt qtd->status = ERROR_DATA_BUFFER_ERROR");
                }
                else if (status & (1<<4)) {
                    ret = 0; //ERROR_BABBLE_DETECTED;
                    Log(Error, "USB EHCI wait interrupt qtd->status = ERROR_BABBLE_DETECTED");
                }
                else if (status & (1<<3)) {
                    ret = 0; //ERROR_NAK;
                    Log(Error, "USB EHCI wait interrupt qtd->status = ERROR_NAK");
                }
                else if (status & (1<<2)) {
                    ret = 0; //ERROR_TIME_OUT;
                    Log(Error, "USB EHCI wait interrupt qtd->status = ERROR_TIME_OUT");
                }
                else {
                    Log(Error, "USB EHCI wait interrupt qtd->status = %x", status);
                    ret = 0; //ERROR_UNKNOWN;
                }
                return ret;
            }
            if ((((status & 0x7FFF0000) >> 16) > 0) && (((status & (3<<8))>>8) == 1)) {
                if ((*(uint32_t*)(virtAddr + EHCI_TD_OFF_ALT_NEXT_QTD_PTR) & 1) == 0) {
                    virtAddr = *(uint32_t*)(virtAddr + EHCI_TD_OFF_ALT_TD_VIRT_PTR);
                    timer = timeout;
                } 
                else
                    return ret;
            } else {
                if ((*(uint32_t*)(virtAddr + EHCI_TD_OFF_NEXT_TD_PTR) & 1) == 0) {
                    virtAddr = *(uint32_t*)(virtAddr + EHCI_TD_OFF_NEXT_TD_VIRT_PTR);
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

bool EHCIController::SetAddress(const uint8_t max_packet, const uint8_t address) {
    
    // allocate enough memory to hold the out_packet, queue and the TD's
    uint32_t packetPhys;
    uint32_t packetVirt = (uint32_t)KernelHeap::allignedMalloc(64 + EHCI_QUEUE_HEAD_SIZE + (2 * EHCI_TD_SIZE), 64, &packetPhys);
    uint32_t queuePhys = packetPhys + 64;
    uint32_t queueVirt = packetVirt + 64;
    uint32_t td0Virt = queueVirt + EHCI_QUEUE_HEAD_SIZE;
    uint32_t td0Phys = queuePhys + EHCI_QUEUE_HEAD_SIZE;
    
    static uint8_t temp[8] = { 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    temp[2] = address;
    
    MemoryOperations::memcpy((void*)packetVirt, temp, sizeof(struct REQUEST_PACKET));
    
    SetupQueueHead(queueVirt, td0Phys, 0, max_packet, 0);
    MakeSetupTransferDesc(td0Virt, td0Phys, packetPhys);
    MakeTransferDesc(td0Virt + EHCI_TD_SIZE, td0Phys + EHCI_TD_SIZE, 0, 0, 0, true, 1, EHCI_TD_PID_IN, max_packet);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForInterrupt(td0Virt, 2000, 0);
    RemoveFromQueue(queueVirt);

    KernelHeap::allignedFree((void*)packetVirt);
    
    return (ret == 1);
}

bool EHCIController::ControlIn(void* targ, const int len, const int max_packet, const uint8_t address) {
    
    // allocate enough memory to hold the packet, queue, and the TD's
    uint32_t packetPhys;
    uint32_t packetVirt = (uint32_t)KernelHeap::allignedMalloc(64 + EHCI_QUEUE_HEAD_SIZE + (16 * EHCI_TD_SIZE), 64, &packetPhys);
    uint32_t queuePhys = packetPhys + 64;
    uint32_t queueVirt = packetVirt + 64;
    uint32_t td0Phys = queuePhys + EHCI_QUEUE_HEAD_SIZE;
    uint32_t td0Virt = queueVirt + EHCI_QUEUE_HEAD_SIZE;
    
    uint32_t bufferPhys;
    uint32_t bufferVirt = (uint32_t)KernelHeap::malloc(256, &bufferPhys);  // get a physical address buffer and then copy from it later
    
    static struct REQUEST_PACKET temp = { STDRD_GET_REQUEST, GET_DESCRIPTOR, ((DEVICE << 8) | 0), 0, 0 };
    temp.length = len;
    
    // copy the request packet from local memory to physical address memory
    MemoryOperations::memcpy((void*)packetVirt, &temp, sizeof(struct REQUEST_PACKET));
    
    bool spd = 0;
    const int last = 1 + ((len + (max_packet-1)) / max_packet);
    
    SetupQueueHead(queueVirt, td0Phys, 0, max_packet, address);
    MakeSetupTransferDesc(td0Virt, td0Phys, packetPhys);
    MakeTransferDesc(td0Virt + EHCI_TD_SIZE, td0Phys + EHCI_TD_SIZE, td0Phys + (last * EHCI_TD_SIZE), bufferPhys, len, false, 1, EHCI_TD_PID_IN, max_packet);
    MakeTransferDesc(td0Virt + (last * EHCI_TD_SIZE), td0Phys + (last * EHCI_TD_SIZE), 0, 0, 0, true, 1, EHCI_TD_PID_OUT, max_packet);
    
    InsertIntoQueue(queueVirt, queuePhys, QH_HS_TYPE_QH);
    int ret = WaitForInterrupt(td0Virt, 2000, &spd);
    RemoveFromQueue(queueVirt);
    KernelHeap::allignedFree((void*)packetVirt);
    
    if (ret == 1) {
        // now copy from the physical buffer to the specified buffer
        MemoryOperations::memcpy(targ, (void*)bufferVirt, len);
        KernelHeap::free((void*)bufferVirt);
        return true;
    }
    
    KernelHeap::allignedFree((void*)bufferVirt);
    return false;
}

uint32_t EHCIController::ReadOpReg(uint32_t reg)
{
    return readMemReg(regBase + operRegsOffset + reg);
}
void EHCIController::WriteOpReg(uint32_t reg, uint32_t val)
{
    writeMemReg(regBase + operRegsOffset + reg, val);
}
/////////
// USB Controller Functions
/////////
bool EHCIController::GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device)
{
    return ControlIn(dev_desc, 18, 64, device->devAddress);
}