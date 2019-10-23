#include <system/drivers/usb/controllers/xhci.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

XHCIController::XHCIController(PCIDevice* device)
: USBController(xHCI), Driver("XHCI USB Controller", "Controller for a XHCI device"), port_info() 
{
    this->pciDevice = device;
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
            Log(Error, "XHCI Did not halt within 20 ms.");
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
            Log(Error, "XHCI Did not reset within 500 ms.");
            return false;
        }
        
        System::pit->Sleep(1);
    }
    System::pit->Sleep(USB_TRSTRCY);

    //Read HCCParams1 register and store its value, now we can read 64 bit values
    hccparams1 = readMemReg(regBase + xHC_CAPS_HCCParams1);

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
    

    //Add ourself to known controllers
    System::usbManager->AddController(this);

    return true;
}
void XHCIController::Setup()
{
    //Write physical address of Context Data Structure
    WriteOpReg64(xHC_OPS_USBDcbaap, 0);

    //Write physical address to Ring Control Register
    WriteOpReg64(xHC_OPS_USBCrcr, 0 | 1);

    //Let the controller know the amount of slots we support
    WriteOpReg(xHC_OPS_USBConfig, 0);//max_slots);

    //Set bit in Device Notification Control
    WriteOpReg(xHC_OPS_USBDnctrl, (1<<1));

    //Clear the status register bits
    WriteOpReg(xHC_OPS_USBStatus, (1<<10) | (1<<4) | (1<<3) | (1<<2));

    //Set and start the Host Controllers schedule
    WriteOpReg(xHC_OPS_USBCommand, (1<<3) | (1<<2) | (1<<0));
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
        Log(Warning, "XHCI Port Reset Timed out");
    
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