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
    
    //Save Register memory base
    this->regBase = BAR0.address;

    //Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, 0x0006);

    //Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)this->regBase, (void*)this->regBase, pageRoundUp(BAR0.size), true, true);

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

    // make a local frame list, and clear it out
    struct OHCI_FRAME our_frame;
    MemoryOperations::memset(&our_frame, 0, sizeof(struct OHCI_FRAME));
        
    // create the control list, locally
    for (int i = 0; i < 16; i++) {
        our_frame.control_ed[i].nexted = (uint32_t) (hccaPhys + ((uint32_t) &our_frame.control_ed[i+1] - (uint32_t) &our_frame));
        our_frame.control_ed[i].flags = (1<<13); // sKip bit
    }
    our_frame.control_ed[15].nexted = 0x00000000;  // mark the last one

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
    writeMemReg(regBase + OHCControlHeadED, (uint32_t)(hccaPhys + ((uint32_t) &our_frame.control_ed[0] - (uint32_t) &our_frame)));
    writeMemReg(regBase + OHCControlCurrentED, 0x00000000);

    //copy our local stack to the real stack
    MemoryOperations::memcpy(hcca, &our_frame, sizeof(struct OHCI_FRAME));
    //movedata(_my_ds(), (uint32_t) &our_frame, hcca_selector, 0, sizeof(struct OHCI_FRAME));

    //Disallow interrupts (we poll the DoneHeadWrite bit instead)
    writeMemReg(regBase + OHCInterruptDisable, 0x80000000);
    
    // Start the controller  
    writeMemReg(regBase + OHCControl, 0x00000690);  // CLE & operational
    
    //Set port power switching
    uint32_t val = readMemReg(regBase + OHCRhDescriptorA);
    //Read Power On To Power Good Time (Is in units of 2 mS)
    uint16_t potpgt = (uint16_t) (((val >> 24) * 2) + 2);  // plus two to make sure we wait long enough
    writeMemReg(regBase + OHCRhDescriptorA, ((val & OHCRhDescriptorA_MASK) & ~(1<<9)) | (1<<8));
    
    // loop through the ports
    int dev_address = 1;
    struct DEVICE_DESC dev_desc;
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
            //  while in the default state.  We must request the first 8 bytes, then reset
            //  the port, set address, then request all 18 bytes.
            CreateStack(&our_frame, 8, 8, ls_device, 0);
            bool good_ret = RequestDesc(&our_frame, 8);
            if (good_ret) {
                int desc_len = our_frame.packet[0];  // get the length of the descriptor (always 18)
                int desc_mps = our_frame.packet[7];  // get the mps of the descriptor
                
                // reset the port again
                if (!ResetPort(i))
                    continue;
                
                // set address
                SetAddress(&our_frame, dev_address, ls_device);
                good_ret = RequestDesc(&our_frame, 0);
                if (!good_ret) {
                    Log(Error, "Error when trying to set device address to %d", dev_address);
                    continue;
                }

                //Create Device
                USBDevice* newDev = new USBDevice();
                newDev->controller = this;
                newDev->devAddress = dev_address;
                newDev->portNum = i;
                newDev->ohciProperties.desc_mps = desc_mps;
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
// create a valid stack frame with our GetDeviceDescriptor tranfser packets
void OHCIController::CreateStack(struct OHCI_FRAME* frame, const int mps, int cnt, const bool ls_device, const int address) {
  
    uint8_t setup_packet[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };
    int i, p, t;
    
    // fill in the setup packet
    * ((uint16_t*) &setup_packet[6]) = cnt;
    MemoryOperations::memcpy(frame->setup, setup_packet, 8);
    
    // clear the return packet
    MemoryOperations::memset(frame->packet, 0, 18);
    
    // create the setup td
    frame->our_tds[0].flags = (14<<28) | (0 << 26) | (2 << 24) | (7<<21) | (TD_DP_SETUP << 19);
    frame->our_tds[0].cbp = hccaPhys + ((uint32_t) &frame->setup[0] - (uint32_t) frame);
    frame->our_tds[0].nexttd = hccaPhys + ((uint32_t) &frame->our_tds[1] - (uint32_t) frame);
    frame->our_tds[0].be = frame->our_tds[0].cbp + 7;
    
    // create the rest of the in tds
    i = 1; p = 0; t = 1;
    while (cnt > 0) {
        frame->our_tds[i].flags = (14<<28) | (0 << 26) | ((2 | (t & 1)) << 24) | (7<<21) | (TD_DP_IN << 19);
        frame->our_tds[i].cbp = hccaPhys + ((uint32_t) &frame->packet[p] - (uint32_t) frame);
        frame->our_tds[i].nexttd = hccaPhys + ((uint32_t) &frame->our_tds[i+1] - (uint32_t) frame);
        frame->our_tds[i].be = frame->our_tds[i].cbp + ((cnt > mps) ? (mps - 1) : (cnt - 1));
        t ^= 1;
        p += mps;
        i++;
        cnt -= mps;
    }
    
    // create the status td
    frame->our_tds[i].flags = (14<<28) | (0 << 26) | (3 << 24) | (0<<21) | (TD_DP_OUT << 19);
    frame->our_tds[i].cbp = 0;
    frame->our_tds[i].nexttd = hccaPhys + ((uint32_t) &frame->our_tds[i+1] - (uint32_t) frame);
    frame->our_tds[i].be = 0;
    
    // create the ED, using one already in the control list
    frame->control_ed[0].flags = (mps << 16) | (0 << 15) | (0 << 14) | (ls_device ? (1<<13) : 0) | (0 << 11) | (0 << 7) | (address & 0x7F);
    frame->control_ed[0].tailp = frame->our_tds[i].nexttd;
    frame->control_ed[0].headp = hccaPhys + ((uint32_t) &frame->our_tds[0] - (uint32_t) frame);
}

void OHCIController::SetAddress(struct OHCI_FRAME* frame, int dev_address, bool ls_device) {

    uint8_t setup_packet[8] = { 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    
    setup_packet[2] = (uint8_t) dev_address;
    
    // fill in the setup packet
    MemoryOperations::memcpy(frame->setup, setup_packet, 8);
    
    // create the setup td
    frame->our_tds[0].flags = (14<<28) | (2<<24) | (7<<21) | (0<<19);
    frame->our_tds[0].cbp = hccaPhys + ((uint32_t) &frame->setup[0] - (uint32_t) frame);
    frame->our_tds[0].nexttd = hccaPhys + ((uint32_t) &frame->our_tds[1] - (uint32_t) frame);
    frame->our_tds[0].be = frame->our_tds[0].cbp + 7;
    
    // create the status td
    frame->our_tds[1].flags = (14<<28) | (3<<24) | (0<<21) | (2<<19);
    frame->our_tds[1].cbp = 0;
    frame->our_tds[1].nexttd = hccaPhys + ((uint32_t) &frame->our_tds[2] - (uint32_t) frame);
    frame->our_tds[1].be = 0;
    
    // create the ED, using one already in the control list
    frame->control_ed[0].flags = 0x00080000 | ((ls_device) ? (1<<13) : 0);
    frame->control_ed[0].tailp = frame->our_tds[1].nexttd;
    frame->control_ed[0].headp = hccaPhys + ((uint32_t) &frame->our_tds[0] - (uint32_t) frame);
}

bool OHCIController::RequestDesc(struct OHCI_FRAME* our_frame, int cnt) {
    int timeout, i;
    bool good_ret;
    
    // clear all the bits in the interrupt status register
    writeMemReg(regBase + OHCInterruptStatus, (1<<30) | 0x7F);
    
    // copy our local stack to the real stack
    MemoryOperations::memcpy(this->hcca, our_frame, sizeof(struct OHCI_FRAME));
    //movedata(_my_ds(), (uint32_t) our_frame, hcca_selector, 0, sizeof(struct OHCI_FRAME));
    
    // set ControlListFilled bit
    writeMemReg(regBase + OHCCommandStatus, (1<<1));

    //TODO: Find alternative to this delay
    System::pit->Sleep(5);
    
    // wait for bit 1 to be set in the status register
    timeout = 2000; // 2 seconds
    while ((readMemReg(pciDevice->portBase + OHCInterruptStatus) & (1<<1)) == 0) {
        System::pit->Sleep(1);
        if (--timeout == 0)
            break;
    }
    if (timeout == 0) {
        Log(Warning, "Bit 1 in the HCInterruptStatus register never was set.");
        return false;
    }
    
    // copy the real stack to our local stack
    MemoryOperations::memcpy(our_frame, hcca, sizeof(struct OHCI_FRAME));
    //movedata(hcca_selector, 0, _my_ds(), (uint32_t) our_frame, sizeof(struct OHCI_FRAME));
    
    // here is were we would read the HCCA.DoneHead field, and go through the list
    //  to see if there were any errors.
    // For the purpose of this example, we will not use the HCCA.DoneHead, but just
    //  scroll through our transfer descriptors in our local stack.
    
    // clear the bit for next time
    writeMemReg(regBase + OHCInterruptStatus, (1<<1));
    
    // calculate how many tds to check.
    if (cnt > 0)
        cnt /= 8;
    cnt += 2;
    
    good_ret = true;
    for (i=0; i<cnt; i++) {
        if ((our_frame->our_tds[i].flags & 0xF0000000) != 0) {
            good_ret = false;
            Log(Error, "our_tds[%d].cc != 0  (%d)", i, (our_frame->our_tds[i].flags & 0xF0000000) >> 28);
            break;
        }
    }
    
    return good_ret;
}
uint32_t OHCIController::HandleInterrupt(uint32_t esp)
{
    Log(Info, "OHCI Interrupt");

    return esp;
}
/////////
// USB Controller Functions
/////////
bool OHCIController::GetDeviceDescriptor(struct DEVICE_DESC* dev_desc, USBDevice* device)
{
    // make a local frame list, and clear it out
    struct OHCI_FRAME our_frame;
    MemoryOperations::memset(&our_frame, 0, sizeof(struct OHCI_FRAME));
        
    // create the control list, locally
    for (int i = 0; i < 16; i++) {
        our_frame.control_ed[i].nexted = (uint32_t) (hccaPhys + ((uint32_t) &our_frame.control_ed[i+1] - (uint32_t) &our_frame));
        our_frame.control_ed[i].flags = (1<<13); // sKip bit
    }
    our_frame.control_ed[15].nexted = 0x00000000;  // mark the last one

    //copy our local stack to the real stack
    MemoryOperations::memcpy(hcca, &our_frame, sizeof(struct OHCI_FRAME));

    CreateStack(&our_frame, device->ohciProperties.desc_mps, 18, device->ohciProperties.ls_device, device->devAddress);
    if (RequestDesc(&our_frame, 18)) {
        MemoryOperations::memcpy(dev_desc, our_frame.packet, sizeof(struct DEVICE_DESC));
        return true;
    }
    return false;
}