#ifdef XHCI_SUPPORTED

#include <system/drivers/usb/controllers/xhci.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

XHCIController::XHCIController(PCIDevice* device)
: USBController(xHCI), Driver("XHCI USB Controller", "Controller for a XHCI device"),
InterruptHandler(IDT_INTERRUPT_OFFSET + device->interrupt)
{
    this->pciDevice = device;
    MemoryOperations::memset(this->port_info, 0, 16 * sizeof(struct S_XHCI_PORT_INFO));
}

bool XHCIController::Initialize()
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

    // Write to the FLADJ register incase the BIOS didn't
    // At the time of this writing, there wasn't a BIOS that supported xHCI yet :-)
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x61, 0x20);

    //Read the version register (just a small safety check)
    if (readMemReg(regBase + xHC_CAPS_IVersion) < 0x95)
        return false;
  
    //If it is a Panther Point device, make sure sockets are xHCI controlled.
    if ((System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 0) == 0x8086) && 
        (System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 2) == 0x1E31) && 
        (System::pci->Read(pciDevice->bus, pciDevice->device, pciDevice->function, 8) == 4)) {
        System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0xD8, 0xFFFFFFFF);
        System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0xD0, 0xFFFFFFFF);
    }

    //Read Register offset
    opRegOffset = (uint8_t) readMemReg(regBase + xHC_CAPS_CapLength);

    //Set Run/Stop bit to clear
    WriteOpReg(xHC_OPS_USBCommand, ReadOpReg(xHC_OPS_USBCommand) & ~(1<<0));

    //Wait for HcHalted bit to be set.
    uint32_t timeout = 20;
    while (!((ReadOpReg(xHC_OPS_USBStatus) & (1<<0)))) {
        if(--timeout == 0) {
            Log(Error, "xHCI Did not halt within 20 ms.");
            return false;
        }
        System::pit->Sleep(1);
    }

    //Set hcreset bit
    //Wait for hcreset bit and CtrlNotReady to clear
    timeout = 500;
    WriteOpReg(xHC_OPS_USBCommand, (1<<1));
    while ((ReadOpReg(xHC_OPS_USBCommand) & (1<<1)) || (ReadOpReg(xHC_OPS_USBCommand) & (1<<11))) {
        if (--timeout == 0) {
            Log(Error, "xHCI Did not reset within 500 ms.");
            return false;
        }
        
        System::pit->Sleep(1);
    }
    System::pit->Sleep(USB_TRSTRCY);

    //Read HCCParams1 register and store its value, now we can read 64 bit values
    hccparams1 = readMemReg(regBase + xHC_CAPS_HCCParams1);
    hccparams2 = readMemReg(regBase + xHC_CAPS_HCCParams2);
    hcsparams1 = readMemReg(regBase + xHC_CAPS_HCSParams1);
    hcsparams2 = readMemReg(regBase + xHC_CAPS_HCSParams2);
    rts_offset = readMemReg(regBase + xHC_CAPS_RTSOFF) & ~0x1F;  // bits 4:0 are reserved
    db_offset = readMemReg(regBase + xHC_CAPS_DBOFF) & ~0x03;    // bits 1:0 are reserved
    context_size = (hccparams1 & (1<<2)) ? 64 : 32;
    uint32_t xECP = (((hccparams1 & 0xFFFF0000) >> 16) * 4);

    //Check for default values of the operational registers
    if(ReadOpReg(xHC_OPS_USBCommand) != 0)
        return false;
    if(ReadOpReg(xHC_OPS_USBDnctrl) != 0)
        return false;
    if(ReadOpReg64(xHC_OPS_USBCrcr) != 0)
        return false;
    if(ReadOpReg64(xHC_OPS_USBDcbaap) != 0)
        return false;
    if(ReadOpReg(xHC_OPS_USBConfig) != 0)
        return false;

    // Turn off legacy support for Keyboard and Mice
    if (!StopLegacy(xECP)) {
        Log(Error, "[xHCI] BIOS did not release Legacy support...");
        return false;
    }

    //Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void XHCIController::Setup()
{
    //Get num_ports from XHCI's HCSPARAMS1 register
	numPorts = (uint8_t) ((hcsparams1 & 0xFF000000) >> 24);
    Log(Info, "[xHCI] Found %d (virtual) root hub ports.", numPorts);

    // Get protocol of each port
    //  Each physical port will have a USB3 and a USB2 PortSC register set.
    //  Most likely a controller will only have one protocol item for each version.
    //   i.e.:  One for USB 3 and one for USB 2, they will not be fragmented.
    //  However, it doesn't state anywhere that it can't be fragmented, so the below
    //   code allows for fragmented protocol items
    uint32_t ext_caps_off = (((hccparams1 & 0xFFFF0000) >> 16) * 4);
    uint32_t next = ext_caps_off;
    uint16_t flags;
    int cnt, offset, ports_usb2 = 0, ports_usb3 = 0;
    
    // find the USB 2.0 ports and mark the port_info byte as USB2 if found
    while (next) {
        next = GetProtoOffset(next, 2, &offset, &cnt, &flags);
        if (cnt) {
            for (int i = 0; i < cnt; i++) {
                port_info[offset + i].offset = ports_usb2++;
                port_info[offset + i].flags = xHCI_PROTO_USB2;
                if (flags & 2)
                    port_info[offset + i].flags |= xHCI_PROTO_HSO;
            }
        }
    }
    
    // find the USB 3.0 ports and mark the port_info byte as USB3 if found
    next = ext_caps_off;
    while (next) {
        next = GetProtoOffset(next, 3, &offset, &cnt, &flags);
        if (cnt) {
            for (int i = 0; i < cnt; i++) {
                port_info[offset + i].offset = ports_usb3++;
                port_info[offset + i].flags = xHCI_PROTO_USB3;
            }
        }
    }
    
    // pair up each USB3 port with it's companion USB2 port
    for (int i = 0; i < numPorts; i++) {
        for (int k = 0; k < numPorts; k++) {
            if ((port_info[k].offset == port_info[i].offset) && ((port_info[k].flags & xHCI_PROTO_INFO) != (port_info[i].flags & xHCI_PROTO_INFO))) {
                port_info[i].other_port_num = k;
                port_info[i].flags |= xHCI_PROTO_HAS_PAIR;
                port_info[k].other_port_num = i;
                port_info[k].flags |= xHCI_PROTO_HAS_PAIR;
            }
        }
    }
    
    // mark all USB3 ports and any USB2 only ports as active, deactivating any USB2 ports that have a USB3 companion
    for (int i = 0; i < numPorts; i++) {
        if (xHCI_IS_USB3_PORT(i) || (xHCI_IS_USB2_PORT(i) && !xHCI_HAS_PAIR(i)))
            port_info[i].flags |= xHCI_PROTO_ACTIVE;
    }

    pageSize = (ReadOpReg(xHC_OPS_USBPageSize) & 0xFFFF) << 12;
    Log(Info, "[xHCI] Page Size = %d", pageSize);
    const uint32_t max_slots = hcsparams1 & 0xFF;
    
    //Allocate Device Context Area
    devContextAreaVirt = (uint32_t)KernelHeap::alignedMalloc(2048, pageSize, &devContextAreaPhys);
    MemoryOperations::memset((void*)devContextAreaVirt, 0, 2048); //And set it to zero's

    //Write physical address of Context Data Structure
    WriteOpReg64(xHC_OPS_USBDcbaap, devContextAreaPhys);

    //Allocate Command Ring
    commandRingVirt = CreateRing(CMND_RING_TRBS);
    commandRingPhys = (uint32_t)VirtualMemoryManager::virtualToPhysical((void*)commandRingVirt);
    commandTrbPhys = commandRingPhys;
    commandTrbVirt = commandRingVirt;
    commandTrbCycle = TRB_CYCLE_ON;

    //Write physical address to Ring Control Register
    WriteOpReg64(xHC_OPS_USBCrcr, commandRingPhys | TRB_CYCLE_ON);

    //Let the controller know the amount of slots we support
    WriteOpReg(xHC_OPS_USBConfig, max_slots);

    //Set bit in Device Notification Control
    WriteOpReg(xHC_OPS_USBDnctrl, (1<<1));

    int maxEventSegments = (1 << ((hcsparams2 & 0x000000F0) >> 4));
    int maxInterrupters = ((hcsparams1 & 0x0007FF00) >> 8);
    uint32_t eventRingAddr = CreateEventRing(4096, &curEventRingAddrVirt);
    curEventRingAddrPhys = (uint32_t)VirtualMemoryManager::virtualToPhysical((void*)curEventRingAddrVirt);
    curEventRingCycle = 1;
    
    //Write the registers
    writePrimaryIntr(xHC_INTERRUPTER_IMAN, (1 << 1) | (1 << 0));  // enable bit & clear pending bit
    writePrimaryIntr(xHC_INTERRUPTER_IMOD, 0);                    // disable throttling
    writePrimaryIntr(xHC_INTERRUPTER_TAB_SIZE, 1);                // count of segments (table size)
    writePrimaryIntr64(xHC_INTERRUPTER_DEQUEUE, (curEventRingAddrPhys | (1 << 3)));
    writePrimaryIntr64(xHC_INTERRUPTER_ADDRESS, (uint32_t)VirtualMemoryManager::virtualToPhysical((void*)eventRingAddr));

    //Clear the status register bits
    WriteOpReg(xHC_OPS_USBStatus, (1<<10) | (1<<4) | (1<<3) | (1<<2));

    //Set and start the Host Controllers schedule
    WriteOpReg(xHC_OPS_USBCommand, (1<<3) | (1<<2) | (1<<0));
    System::pit->Sleep(100);

    // loop through the ports, starting with the USB3 ports
    for (int i = 0; i < numPorts; i++) {
        if (xHCI_IS_USB3_PORT(i) && xHCI_IS_ACTIVE(i)) {
            // power and reset the port
            if (ResetPort(i)) {
                // if the reset was good, get the descriptor
                // if the reset was bad, the reset routine will mark this port as inactive,
                //  and mark the USB2 port as active.
                Log(Info, "Getting USB3 Descriptor of port %d", i);
                GetDescriptor(i);
            }
        }
    }
    
    // now the USB2 ports
    for (int i = 0; i < numPorts; i++) {
        if (xHCI_IS_USB2_PORT(i) && xHCI_IS_ACTIVE(i)) {
            // power and reset the port
            if (ResetPort(i)) {
                // if the reset was good, get the descriptor
                Log(Info, "Getting USB2 Descriptor of port %d", i);
                GetDescriptor(i);
            }
        }
    }
}
uint32_t XHCIController::HandleInterrupt(uint32_t esp)
{ 
    Log(Info, "[xHCI] Interrupt");

    // acknowledge interrupt (status register first)
    // clear the status register bits
    WriteOpReg(xHC_OPS_USBStatus, ReadOpReg(xHC_OPS_USBStatus));
    
    const uint32_t dword = readPrimaryIntr(xHC_INTERRUPTER_IMAN);
    if ((dword & 3) == 3) {
        // acknowledge the interrupter's IP bit being set
        writePrimaryIntr(xHC_INTERRUPTER_IMAN, dword | 3);
        
        // do the work
        struct xHCI_TRB event, org;
        uint32_t org_address;
        uint32_t last_addr = curEventRingAddrVirt;
        GetTrb(&event, curEventRingAddrVirt);
        
        while ((event.command & 1) == curEventRingAddrPhys) {
            if ((event.command & (1<<2)) == 0) {
                switch (TRB_GET_COMP_CODE(event.status)) {
                case TRB_SUCCESS:
                    switch (TRB_GET_TYPE(event.command)) {
                    // Command Completion Event
                    case COMMAND_COMPLETION:
                        org_address = (uint32_t) event.param;
                        GetTrb(&org, org_address);
                        switch (TRB_GET_TYPE(org.command)) {
                            case ENABLE_SLOT:
                                org.command &= 0x00FFFFFF;
                                org.command |= (event.command & 0xFF000000); // return slot ID (1 based)
                                org.status = event.status;
                                break;
                                
                            default:
                                org.status = event.status;
                                break;
                        }
                        
                        // mark the command as done
                        org.status |= XHCI_IRQ_DONE;
                        // and write it back
                        SetTrb(&org, org_address);
                        break;
                    }
                    break;
                }
                
                // mark the TRB as done
            } else {
                switch (TRB_GET_TYPE(event.command)) {
                    case TRANS_EVENT: // If SPD was encountered in this TD, comp_code will be SPD, else it should be SUCCESS (specs 4.10.1.1)
                        *(uint32_t*)((uint32_t) event.param) = (event.status | XHCI_IRQ_DONE); // return code + bytes *not* transferred
                        break;
                        
                    default:
                        ;
                }
            }
            
            // get next one
            last_addr = curEventRingAddrVirt;
            curEventRingAddrVirt += sizeof(struct xHCI_TRB);
            curEventRingAddrPhys += sizeof(struct xHCI_TRB);
            GetTrb(&event, curEventRingAddrVirt);
        }
        
        // advance the dequeue pointer (clearing the busy bit)
        writePrimaryIntr64(xHC_INTERRUPTER_DEQUEUE, last_addr | (1<<3));
    }
    return esp;
}

bool XHCIController::ResetPort(const int port) {
    bool ret = false;
    uint32_t HCPortStatusOff = xHC_OPS_USBPortSt + (port * 16);
    uint32_t val;
    
    // power the port?
    if ((ReadOpReg(HCPortStatusOff + xHC_Port_PORTSC) & (1<<9)) == 0) {
        WriteOpReg(HCPortStatusOff + xHC_Port_PORTSC, (1<<9));
        System::pit->Sleep(20);
        if ((ReadOpReg(HCPortStatusOff + xHC_Port_PORTSC) & (1<<9)) == 0)
            return false;  // return bad reset.
    }
    
    // we need to make sure that the status change bits are clear
    WriteOpReg(HCPortStatusOff + xHC_Port_PORTSC, (1<<9) | xHC_PortUSB_CHANGE_BITS);
    
    // set bit 4 (USB2) or 31 (USB3) to reset the port
    if (xHCI_IS_USB3_PORT(port))
        WriteOpReg(HCPortStatusOff + xHC_Port_PORTSC, (1<<9) | (1<<31));
    else
        WriteOpReg(HCPortStatusOff + xHC_Port_PORTSC, (1<<9) | (1<<4));
    
    // wait for bit 21 to set
    int timeout = 500;
    while (timeout) {
        val = ReadOpReg(HCPortStatusOff + xHC_Port_PORTSC);
        if (val & (1<<21))
            break;
        timeout--;
        System::pit->Sleep(1);
    }
    
    // if we didn't time out
    if (timeout > 0) {
        // reset recovery time
        System::pit->Sleep(USB_TRHRSI);
        
        // if after the reset, the enable bit is non zero, there was a successful reset/enable
        val = ReadOpReg(HCPortStatusOff + xHC_Port_PORTSC);
        
        if (val & (1<<1)) {
            // clear the status change bit(s)
            WriteOpReg(HCPortStatusOff + xHC_Port_PORTSC, (1<<9) | xHC_PortUSB_CHANGE_BITS);
            
            // success
            ret = true;
        }
    } else
        Log(Warning, "[xHCI] Port Reset Timed out port=%d", port);
    
    // if we have a successful USB2 reset, we need to make sure this port is marked active,
    //  and if it has a paired port, it is marked inactive
    if ((ret == true) && xHCI_IS_USB2_PORT(port)) {
        port_info[port].flags |= xHCI_PROTO_ACTIVE;
        if (port_info[port].flags & xHCI_PROTO_HAS_PAIR)
            port_info[port_info[port].other_port_num].flags &= ~xHCI_PROTO_ACTIVE;
    }
    
    // if error resetting USB3 protocol, deactivate this port and activate the paired USB2 port.
    //  it will be paired since all USB3 ports must be USB2 compatible.
    if (!ret && xHCI_IS_USB3_PORT(port)) {
        port_info[port].flags &= ~xHCI_PROTO_ACTIVE;
        port_info[port_info[port].other_port_num].flags |= xHCI_PROTO_ACTIVE;
    }
    
    return ret;
}

bool XHCIController::StopLegacy(uint32_t xECP) {
    if(xECP != 0)
    {
        uint8_t xECP_id = 0;

        while (xECP) // 00h indicates end of the ext. cap. list.
        {
            xECP_id = *(uint8_t*)(regBase + xECP);
            if (xECP_id == 1)
                break;

            uint8_t offset = *(uint8_t*)(regBase + xECP + 1);
            if (!offset)
                xECP = 0;
            else
                xECP += offset << 2;
        }
        uint16_t BIOSownedSemaphore = xECP + 2; // R/W - only Bit 16 (Bit 23:17 Reserved, must be set to zero)
        uint16_t OSownedSemaphore = xECP + 3;   // R/W - only Bit 24 (Bit 31:25 Reserved, must be set to zero)
        uint16_t USBLEGCTLSTS = xECP + 4;       // USB Legacy Support Control/Status

        // Legacy-Support-EC found? BIOS-Semaphore set?
        if (xECP_id == 1 && *(uint8_t*)(regBase + BIOSownedSemaphore) & 0x01)
        {
            //Set Semaphore
            *(uint8_t*)(regBase + OSownedSemaphore) = 0x01;

            uint32_t timeOut = 250;
            while(timeOut && !((*(uint8_t*)(regBase + BIOSownedSemaphore) & 0x01) == 0)) {
                timeOut--;
                System::pit->Sleep(1);
            }
            if(timeOut == 0) {
                Log(Error, "[xHCI] BIOS-Semaphore still set.");
                return false;
            }

            timeOut = 250;
            while(timeOut && !((*(uint8_t*)(regBase + OSownedSemaphore) & 0x01) != 0)) {
                timeOut--;
                System::pit->Sleep(1);
            }
            if(timeOut == 0) {
                Log(Error, "[xHCI] OS-Semaphore still cleared.");
                return false;
            }

            // USB SMI Enable R/W. 0=Default. The OS tries to set SMI to disabled in case that BIOS bit stays at one.
            *(uint32_t*)(regBase + USBLEGCTLSTS) = 0x00000000; // USB SMI disabled
            return true;
        }
        else
            Log(Info, "[xHCI] BIOS did not own the xHCI. No action needed.");
    }
    return true;
}

uint32_t XHCIController::GetProtoOffset(uint32_t list_off, const int version, int* offset, int* count, uint16_t* flags) {
    uint32_t next;
    *count = 0;  // mark that there isn't any to begin with

    do {
        // calculate next item position
        uint8_t item_next = *(uint8_t*)(regBase + list_off + 1);
        next = (item_next) ? (list_off + (item_next * 4)) : 0;
        
        // is this a protocol item and if so, is it the version we are looking for?
        if ((*(uint8_t*)(regBase + list_off + 0) == xHC_xECP_ID_PROTO) && (*(uint8_t*)(regBase + list_off + 3) == version)) {
            *offset = *(uint8_t*)(regBase + list_off + 8) - 1;  // make it zero based
            *count = *(uint8_t*)(regBase + list_off + 9);
            *flags = *(uint16_t*)(regBase + list_off + 10) & 0x0FFF;
            return next;
        }
        
        // point to next item
        list_off = next;
    } while (list_off);
    
    // return no more
    return 0;
}

uint32_t XHCIController::CreateRing(const int trbs) {
    const uint32_t addr = (uint32_t)KernelHeap::alignedMalloc(trbs * sizeof(struct xHCI_TRB), 0x10000);
    MemoryOperations::memset((void*)addr, 0, trbs * sizeof(struct xHCI_TRB));
    
    // make the last one a link TRB to point to the first one
    uint32_t pos = addr + ((trbs - 1) * sizeof(struct xHCI_TRB));
    *(uint64_t*)(pos + 0) = addr;           // param
    *(uint32_t*)(pos +  8) = (0 << 22) | 0;  // status
    *(uint32_t*)(pos + 12) = TRB_LINK_CMND;  // command
    
    return addr;
}

uint32_t XHCIController::CreateEventRing(const int trbs, uint32_t* ret_addr) {
    // Please note that 'trbs' should be <= 4096 or you will need to make multiple segments
    // I only use one here.
    const uint32_t table_addr = (uint32_t)KernelHeap::alignedMalloc(64, 64);  // min 16 bytes on a 64 byte alignment, no boundary requirements
    MemoryOperations::memset((void*)table_addr, 0, 64);
    const uint32_t addr = (uint32_t)KernelHeap::alignedMalloc((trbs * sizeof(struct xHCI_TRB)), 0x10000); // 16 * trbs, 64 byte alignment, 64k boundary
    MemoryOperations::memset((void*)addr, 0, trbs * sizeof(struct xHCI_TRB));

    // we only use 1 segment for this example
    *(uint64_t*)(table_addr) = addr;
    *(uint32_t*)(table_addr + 8) = trbs;  // count of TRB's
    *(uint32_t*)(table_addr + 12) = 0;
    
    *ret_addr = addr;
    return table_addr;
}
bool XHCIController::GetDescriptor(const int port) {
    uint32_t dword;
    uint32_t HCPortStatusOff = xHC_OPS_USBPortSt + (port * 16);
    struct DEVICE_DESC dev_desc;
    
    // port has been reset, and is ready to be used
    // we have a port that has a device attached and is ready for data transfer.
    // so lets create our stack and send it along.
    dword = ReadOpReg(HCPortStatusOff + xHC_Port_PORTSC);
    int speed = ((dword & (0xF << 10)) >> 10); // FULL = 1, LOW = 2, HI = 3, SS = 4
    /*
    * Some devices will only send the first 8 bytes of the device descriptor
    *  while in the default state.  We must request the first 8 bytes, then reset
    *  the port, set address, then request all 18 bytes.
    */
    
    // send the initialize and enable slot command
    int max_packet;
    
    // send the command and wait for it to return
    struct xHCI_TRB trb;
    trb.param = 0;
    trb.status = 0;
    trb.command = TRB_SET_STYPE(0) | TRB_SET_TYPE(ENABLE_SLOT);
    if (SendCommand(&trb, true))
        return false;
    
    // once we get the interrupt, we can get the slot_id
    uint32_t slot_id = TRB_GET_SLOT(trb.command);
    
    // if the slot id > 0, we have a valid slot id
    if (slot_id > 0) {
        // calculate initial Max Packet Size (xHCI requires this or will not set address)
        switch (speed) {
            case xHCI_SPEED_LOW:
                max_packet = 8;
                break;
            case xHCI_SPEED_FULL:
            case xHCI_SPEED_HI:
                max_packet = 64;
                break;
            case xHCI_SPEED_SUPER:
                max_packet = 512;
                break;
        }
        /*
        // initialize the device/slot context
        uint32_t slot_addr = xhci_initialize_slot(slot_id, port, speed, max_packet);
        // send the address_device command
        xhci_set_address(slot_addr, slot_id, true);
        // now send the "get_descriptor" packet (get 8 bytes)
        xhci_control_in(&dev_desc, 8, slot_id, max_packet);
        
        // TODO: if the dev_desc.max_packet was different than what we have as max_packet,
        //       you would need to change it here and in the slot context by doing a
        //       evaluate_slot_context call.
        
        // reset the port
        ResetPort(port);
        
        // send set_address_command again
        xhci_set_address(slot_addr, slot_id, false);
        
        // get the whole packet.
        xhci_control_in(&dev_desc, 18, slot_id, max_packet);
        
        // print the descriptor
        Log(Info, "Found Device Descriptor:"
                "\n                 len: %d"
                "\n                type: %d"
                "\n             version: %b.%b"
                "\n               class: %d"
                "\n            subclass: %d"
                "\n            protocol: %d"
                "\n     max packet size: %d"
                "\n           vendor id: %w"
                "\n          product id: %w"
                "\n         release ver: %d%d.%d%d"
                "\n   manufacture index: %d (index to a string)"
                "\n       product index: %d"
                "\n        serial index: %d"
                "\n   number of configs: %d",
                dev_desc.len, dev_desc.type, dev_desc.usb_ver >> 8, dev_desc.usb_ver & 0xFF, dev_desc._class, dev_desc.subclass, 
                dev_desc.protocol, dev_desc.max_packet_size, dev_desc.vendorid, dev_desc.productid, 
                (dev_desc.device_rel & 0xF000) >> 12, (dev_desc.device_rel & 0x0F00) >> 8,
                (dev_desc.device_rel & 0x00F0) >> 4,  (dev_desc.device_rel & 0x000F) >> 0,
                dev_desc.manuf_indx, dev_desc.prod_indx, dev_desc.serial_indx, dev_desc.configs);
        */
    }
    
    return true;
}
// inserts a command into the command ring at the current command trb location
// returns TRUE if timed out
bool XHCIController::SendCommand(struct xHCI_TRB* trb, const bool ring_it) {
    // we monitor bit 31 in the command dword
    uint32_t org_trb_addr = commandTrbVirt;
    
    // must write param and status fields to the ring before the command field.
    *(uint64_t*)(commandTrbVirt) = trb->param;                       // param
    *(uint32_t*)(commandTrbVirt +  8) = trb->status;                   // status
    *(uint32_t*)(commandTrbVirt + 12) = trb->command | commandTrbCycle; // command
    
    commandTrbVirt += sizeof(struct xHCI_TRB);
    commandTrbPhys += sizeof(struct xHCI_TRB);
    
    // if the next trb is the link trb, then move to the first TRB
    // ** for this example, we assume that we are moving to the first TRB in the command ring **
    uint32_t cmnd = *(uint32_t*)(commandTrbVirt + 12);
    if (TRB_GET_TYPE(cmnd) == LINK) {
        *(uint32_t*)(commandTrbVirt + 12, (cmnd & ~1) | commandTrbCycle);
        commandTrbVirt = commandRingVirt;
        commandTrbPhys = commandRingPhys;
        commandTrbCycle ^= 1;
    }
    
    if (ring_it) {
        WriteDoorbell(0, 0);   // ring the doorbell
        
        // Now wait for the interrupt to happen
        // We use bit 31 of the command dword since it is reserved
        int timer = 2000;
        while (timer && (*(uint32_t*)(org_trb_addr + 8) & XHCI_IRQ_DONE) == 0) {
            System::pit->Sleep(1);
            timer--;
        }
        if (timer == 0) {
            Log(Error, "[xHCI] Command Interrupt wait timed out.");
            return true;
        } else {
            GetTrb(trb, org_trb_addr);  // retrieve the trb data
            trb->status &= ~XHCI_IRQ_DONE;    // clear off the done bit
        }
    }
    
    return false;
}
void XHCIController::GetTrb(struct xHCI_TRB* trb, const uint32_t address) {
    trb->param =   *(uint64_t*)(address);
    trb->status =  *(uint32_t*)(address +  8);
    trb->command = *(uint32_t*)(address + 12);
}

void XHCIController::SetTrb(struct xHCI_TRB* trb, const uint32_t address) {
    *(uint64_t*)(address) = trb->param;
    *(uint32_t*)(address +  8) = trb->status;
    *(uint32_t*)(address + 12) = trb->command;
}

void XHCIController::WriteOpReg(const uint32_t offset, const uint32_t val) {
    writeMemReg(regBase + opRegOffset + offset, val);
}

void XHCIController::WriteOpReg64(const uint32_t offset, const uint64_t val) {
    writeMemReg(regBase + opRegOffset + offset, val);
    if (hccparams1 & 1)
        writeMemReg(regBase + opRegOffset + offset + 4, (uint32_t) (val >> 32));
}

uint32_t XHCIController::ReadOpReg(const uint32_t offset) {
    return readMemReg(regBase + opRegOffset + offset);
}

uint64_t XHCIController::ReadOpReg64(const uint32_t offset) {
    if (hccparams1 & 1)
        return (readMemReg(regBase + opRegOffset + offset) | ((uint64_t) readMemReg(regBase + opRegOffset + offset + 4) << 32));
    else
        return readMemReg(regBase + opRegOffset + offset);
}

void XHCIController::WriteDoorbell(const uint32_t slot_id, const uint32_t val) {
    *(uint32_t*)(regBase + db_offset + (slot_id * sizeof(uint32_t))) = val; // ring a doorbell
}
void XHCIController::writePrimaryIntr(const uint32_t offset, const uint32_t val) {
    *(uint32_t*)(regBase + (rts_offset + 0x20) + offset) = val;
}

void XHCIController::writePrimaryIntr64(const uint32_t offset, const uint64_t val) {
    *(uint32_t*)(regBase + (rts_offset + 0x20) + offset) = val;
    if (hccparams1 & 1)
        *(uint32_t*)(regBase + (rts_offset + 0x20) + offset + 4) = (uint32_t)(val >> 32);
}

uint32_t XHCIController::readPrimaryIntr(const uint32_t offset) {
    return *(uint32_t*)(regBase + (rts_offset + 0x20) + offset);
}

uint64_t XHCIController::readPrimaryIntr64(const uint32_t offset) {
    if (hccparams1 & 1)
        return (*(uint32_t*)(regBase + (rts_offset + 0x20) + offset) |((uint64_t) *(uint32_t*)(regBase + (rts_offset + 0x20) + offset + 4) << 32));
    else
        return *(uint32_t*)(regBase + (rts_offset + 0x20) + offset);
}
#endif