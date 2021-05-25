#include <system/drivers/disk/ide.h>
#include <system/system.h>
#include <system/tasking/scheduler.h>
#include <system/tasking/lock.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

IDEInterruptHandler::IDEInterruptHandler(IDEController* parent, uint8_t interrupt)
: InterruptHandler(IDT_INTERRUPT_OFFSET + interrupt)
{
    Log(Info, "IDEController adding interrupt handler for int %d", interrupt);
    this->target = parent;
}

uint32_t IDEInterruptHandler::HandleInterrupt(uint32_t esp)
{
    if(this->target != 0)
        this->target->HandleIRQ(esp);

    return esp;
}



IDEController::IDEController(PCIDevice* device)
: Driver("PCI IDE Controller", "PCI IDE Controller"),
  DiskController()
{
    this->pciDevice = device;
    this->devices = List<IDEDevice*>();
}

uint8_t IDEController::ReadRegister(uint8_t channel, uint8_t reg)
{
    uint8_t result = 0;
    
    if (reg < 0x08)
        result = inportb(channels[channel].commandReg + reg);
    else if (reg < 0x0C)
        result = inportb(channels[channel].commandReg + reg - 0x06);
    else if (reg < 0x0E)
        result = inportb(channels[channel].controlReg + reg - 0x0C);
    else if (reg < 0x16)
        result = inportb(channels[channel].bmideReg + reg - 0x0E);
    
    return result;
}

void IDEController::WriteRegister(uint8_t channel, uint8_t reg, uint8_t data)
{    
    if (reg < 0x08)
        outportb(channels[channel].commandReg + reg, data);
    else if (reg < 0x0C)
        outportb(channels[channel].commandReg + reg - 0x06, data);
    else if (reg < 0x0E)
        outportb(channels[channel].controlReg + reg - 0x0C, data);
    else if (reg < 0x16)
        outportb(channels[channel].bmideReg + reg - 0x0E, data);
}

const void IDEController::Wait400NS(uint8_t channel)
{
    inportb(channels[channel].controlReg);
    inportb(channels[channel].controlReg);
    inportb(channels[channel].controlReg);
    inportb(channels[channel].controlReg);
}

bool IDEController::WaitForClear(uint8_t channel, uint8_t reg, uint8_t bits, uint32_t timeout, bool yield)
{
	while (timeout--) {
		if ((this->ReadRegister(channel, reg) & bits) == 0)
			return true;
		
        if(yield && System::scheduler && System::scheduler->Enabled)
            System::scheduler->ForceSwitch();
        else
            System::pit->Sleep(1);
	}
    //Log(Warning, "IDE Wait for clear timed out, status = %b", this->ReadRegister(channel, IDE_REG_ALTSTATUS));
	return false;
}
bool IDEController::WaitForSet(uint8_t channel, uint8_t reg, uint8_t bits, uint32_t timeout, bool yield)
{
	while (timeout--) {
		if ((this->ReadRegister(channel, reg) & bits) == bits)
			return true;
		
        if(yield && System::scheduler && System::scheduler->Enabled)
            System::scheduler->ForceSwitch();
        else
            System::pit->Sleep(1);
	}
	return false;
}

const void IDEController::SetChannelInterruptEnable(uint8_t channel, bool enable)
{
    this->WriteRegister(channel, IDE_REG_CONTROL, enable ? IDE_CTRL_IE : IDE_CTRL_ID);
}

const void IDEController::SetDeviceFeature(uint8_t channel, uint8_t feature, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4)
{
    this->WriteRegister(channel, IDE_REG_FEATURES, feature);
    this->WriteRegister(channel, IDE_REG_SECCOUNT0, arg1);
    this->WriteRegister(channel, IDE_REG_LBA0, arg2);
    this->WriteRegister(channel, IDE_REG_LBA1, arg3);
    this->WriteRegister(channel, IDE_REG_LBA2, arg4);
    this->WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_SETFEATURE);

    if(!this->Polling(channel, false))
        Log(Warning, "IDEController::SetDeviceFeature failed");
}

const void IDEController::SetCountAndLBA(uint8_t channel, uint16_t count, uint32_t lba, bool extended)
{
    if(extended) {
        // Setup second set of LBA Registers
        this->WriteRegister(channel, IDE_REG_SECCOUNT1, (count & 0xFF00) >> 8);
        this->WriteRegister(channel, IDE_REG_LBA3, (lba & 0xFF000000) >> 24);
        this->WriteRegister(channel, IDE_REG_LBA4, 0);
        this->WriteRegister(channel, IDE_REG_LBA5, 0);
    }

    // Setup first set of LBA Registers
    this->WriteRegister(channel, IDE_REG_SECCOUNT0, count & 0xFF);
    this->WriteRegister(channel, IDE_REG_LBA0, (lba & 0x00000FF) >> 0);
    this->WriteRegister(channel, IDE_REG_LBA1, (lba & 0x000FF00) >> 8);
    this->WriteRegister(channel, IDE_REG_LBA2, (lba & 0x0FF0000) >> 16);
}

const void IDEController::PrepareSCSI(uint8_t command, uint32_t lba, uint16_t count, bool dma)
{
    this->atapiPacket[ 0] = command;
    this->atapiPacket[ 1] = 0; //dma ? 1 : 0;
    this->atapiPacket[ 2] = (lba >> 24) & 0xFF;
    this->atapiPacket[ 3] = (lba >> 16) & 0xFF;
    this->atapiPacket[ 4] = (lba >> 8) & 0xFF;
    this->atapiPacket[ 5] = (lba >> 0) & 0xFF;
    this->atapiPacket[ 6] = 0x0;
    this->atapiPacket[ 7] = 0x0;
    this->atapiPacket[ 8] = count >> 8;
    this->atapiPacket[ 9] = count & 0xFF;
    this->atapiPacket[10] = 0x0;
    this->atapiPacket[11] = 0x0;

    if(command == ATAPI_CMD_EJECT)
        this->atapiPacket[ 4] = 0x02;
}

const bool IDEController::Polling(uint8_t channel, bool checkError)
{
    // Delay 400 nanosecond for BSY to be set:
    this->Wait400NS(channel);
    
    // Wait for BSY to be cleared
    if(!this->WaitForClear(channel, IDE_REG_ALTSTATUS, IDE_SR_BSY, IDE_TIMEOUT))
        return false;
    
    if (checkError) {
        uint8_t state = this->ReadRegister(channel, IDE_REG_ALTSTATUS);
    
        // Check For Errors
        if (state & IDE_SR_ERR)
            return false; // Error.
    
        // Check If Device fault
        if (state & IDE_SR_DF)
            return false; // Device Fault.
    
        // Check DRQ
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & IDE_SR_DRQ) == 0)
            return false; // DRQ should be set  
    }
    
    return true;
}

const void IDEController::PIOReadData(uint8_t channel, bool withIORDY, uint8_t* buffer, uint32_t bytes)
{
    if(withIORDY)
        inportsm(this->channels[channel].commandReg + IDE_REG_DATA, buffer, bytes / sizeof(uint16_t));
    else {
        uint16_t* dataPtr = (uint16_t*)buffer;
        while (bytes >= 2) {
            if(!WaitForClear(channel, IDE_REG_ALTSTATUS, IDE_SR_BSY, IDE_TIMEOUT)
              || !WaitForSet(channel, IDE_REG_ALTSTATUS, IDE_SR_DRQ, IDE_TIMEOUT))
                Log(Error, "IDEController::PIOReadData timeout waiting for BSY and DRQ");

            *dataPtr++ = inportw(this->channels[channel].commandReg + IDE_REG_DATA);
            bytes -= 2;
        }
    }
}

const void IDEController::PIOWriteData(uint8_t channel, bool withIORDY, uint8_t* buffer, uint32_t bytes)
{
    if (withIORDY)
        outportsm(this->channels[channel].commandReg + IDE_REG_DATA, buffer, bytes / sizeof(uint16_t));
    else {
        uint16_t* dataPtr = (uint16_t*)buffer;
        while (bytes >= 2) {
            if(!WaitForSet(channel, IDE_REG_ALTSTATUS, IDE_SR_DRQ, IDE_TIMEOUT))
                Log(Error, "IDEController::PIOReadData timeout waiting for DRQ");

            outportw(this->channels[channel].commandReg + IDE_REG_DATA, *dataPtr++);
            bytes -= 2;
        }
    }
}

const bool IDEController::SendPacketCommand(uint8_t channel, uint8_t cmd, uint32_t lba, uint16_t count, bool dma, bool iordy)
{
    // Inform the Controller which mode we use
    this->WriteRegister(channel, IDE_REG_FEATURES, dma ? 1 : 0);

    // Tell the Controller the size of buffer and set other registers to 0
    this->WriteRegister(channel, IDE_REG_SECCOUNT0, 0);
    this->WriteRegister(channel, IDE_REG_LBA0, 0);
    this->WriteRegister(channel, IDE_REG_LBA1, ATAPI_SECTOR_SIZE & 0xFF);   // Lower Byte of Sector Size.
    this->WriteRegister(channel, IDE_REG_LBA2, ATAPI_SECTOR_SIZE >> 8);     // Upper Byte of Sector Size.

    // Disable interrupts
    this->SetChannelInterruptEnable(channel, false);

    // Send the Packet Command
    this->WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_PACKET);

    // Prepare SCSI Packet
    this->PrepareSCSI(cmd, lba, count, dma);

    // Wait for busy to be set
    if(!this->Polling(channel, true))
        return false;

    // Enable interrupts
    this->SetChannelInterruptEnable(channel, true);

    // Write packet data to data port
    this->PIOWriteData(channel, iordy, this->atapiPacket, 12);

    return true;
}

void IDEController::HandleIRQ(uint32_t esp)
{
    /*uint8_t status1 =*/ this->ReadRegister(IDE_CHANNEL_PRIMARY, IDE_REG_STATUS);
    /*uint8_t status2 =*/ this->ReadRegister(IDE_CHANNEL_SECONDARY, IDE_REG_STATUS);

    //Log(Info, "IDE Interrupt (%b, %b)", status1, status2);
 
    /*
    // Reset Interrupt Status
    if(status1 & (1<<2))
        this->WriteRegister(IDE_CHANNEL_PRIMARY, IDE_REG_STATUS, (1<<2));

    if(status2 & (1<<2))
        this->WriteRegister(IDE_CHANNEL_SECONDARY, IDE_REG_STATUS, (1<<2));
    */

    this->irqState = true;
}

const void IDEController::WaitForIRQ()
{
    while(!this->irqState)
        if(System::scheduler && System::scheduler->Enabled)
            System::scheduler->ForceSwitch();
        else
            asm("hlt"); // Might as well offload the CPU
    
    this->irqState = false;
}

bool IDEController::Initialize()
{
    uint8_t mode = this->pciDevice->programmingInterfaceID & 0b1111;

    Log(Info, "Initializing PCI-IDE Controller (Mode = %b)", mode);

    // Get Base Address Registers
    auto BAR0 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 0);
    auto BAR1 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 1);
    auto BAR2 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 2);
    auto BAR3 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 3);
    auto BAR4 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 4);

    // Setup registers
    if(mode & (1<<0)) { // Primary channel is operating in Native-PCI Mode
        this->channels[IDE_CHANNEL_PRIMARY].commandReg = BAR0.address;
        this->channels[IDE_CHANNEL_PRIMARY].controlReg = BAR1.address;
    }
    else { // Primary channel is operating in Compatibility Mode
        this->channels[IDE_CHANNEL_PRIMARY].commandReg = 0x1F0;
        this->channels[IDE_CHANNEL_PRIMARY].controlReg = 0x3F6;
    }
    if(mode & (1<<2)) { // Secondary channel is operating in Native-PCI Mode
        this->channels[IDE_CHANNEL_SECONDARY].commandReg = BAR2.address;
        this->channels[IDE_CHANNEL_SECONDARY].controlReg = BAR3.address;
    }
    else { // Secondary channel is operating in Compatibility Mode
        this->channels[IDE_CHANNEL_SECONDARY].commandReg = 0x170;
        this->channels[IDE_CHANNEL_SECONDARY].controlReg = 0x376;
    }

    // Also set Bus Master addres for both channels
    this->channels[IDE_CHANNEL_PRIMARY].bmideReg = BAR4.address;
    this->channels[IDE_CHANNEL_SECONDARY].bmideReg = BAR4.address + 8;

    Log(Info, "IDE Registers -> (%x, %x) (%x, %x) (%x, %x)",
            this->channels[IDE_CHANNEL_PRIMARY].commandReg,
            this->channels[IDE_CHANNEL_PRIMARY].controlReg,
            this->channels[IDE_CHANNEL_SECONDARY].commandReg,
            this->channels[IDE_CHANNEL_SECONDARY].controlReg,
            this->channels[IDE_CHANNEL_PRIMARY].bmideReg,
            this->channels[IDE_CHANNEL_SECONDARY].bmideReg);

    // Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, (1<<2) | (1<<0));

    // Setup interrupt handlers
    this->interruptHandlers[IDE_CHANNEL_PRIMARY] = new IDEInterruptHandler(this, (mode & (1<<0)) ? this->pciDevice->interrupt : 14);
    if((mode & (1<<2)) == 0) // Secondary channel is operating in Compatibility mode
        this->interruptHandlers[IDE_CHANNEL_SECONDARY] = new IDEInterruptHandler(this, 15);
    
    else if((mode & (1<<0)) == 0) // Secondary channel runs in native mode and we do not have a generic interrupt handler yet (no need to have 2)
        this->interruptHandlers[IDE_CHANNEL_SECONDARY] = new IDEInterruptHandler(this, this->pciDevice->interrupt);

    // Start device detection, we have possibly 2 drives per channel
    for (int channel = 0; channel < 2; channel++)
    {
        for (int device = 0; device < 2; device++)
        {   
            Log(Info, "IDEController checking device %d:%d", channel, device);

            // Type of this device
            uint8_t type = IDE_ATA; 

            // Select Drive:
            this->WriteRegister(channel, IDE_REG_HDDEVSEL, 0xA0 | (device << 4));
            this->Wait400NS(channel);

            // Check if device exists
            if (this->ReadRegister(channel, IDE_REG_ALTSTATUS) == 0xFF)
                continue; // Go to next drive

            // First wait for busy to be cleared (likely not active, but just to be sure)
            if(!this->WaitForClear(channel, IDE_REG_ALTSTATUS, IDE_SR_BSY, IDE_TIMEOUT))
                continue;

            // Set unused variables to 0
            this->WriteRegister(channel, IDE_REG_SECCOUNT0, 0);
            this->WriteRegister(channel, IDE_REG_LBA0, 0);
            this->WriteRegister(channel, IDE_REG_LBA1, 0);
            this->WriteRegister(channel, IDE_REG_LBA2, 0);

            // Disable IRQ's
            this->SetChannelInterruptEnable(channel, false);
    
            // Send ATA Identify Command:
            this->WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_IDENTIFY);
            System::pit->Sleep(10);
    
            // Check if device exists
            if (this->ReadRegister(channel, IDE_REG_ALTSTATUS) == 0)
                continue; // Go to next drive

            bool canBeATAPI = false;
            while(1) {
                uint8_t status = this->ReadRegister(channel, IDE_REG_ALTSTATUS);
                if (status & IDE_SR_ERR) { // If Err, Device is not ATA.
                    canBeATAPI = true;
                    break;
                } 
                if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRQ)) 
                    break; // Everything is right.
            }
    
            // Check for ATAPI Devices:
            if (canBeATAPI) {
                uint8_t cl = this->ReadRegister(channel, IDE_REG_LBA1);
                uint8_t ch = this->ReadRegister(channel, IDE_REG_LBA2);
    
                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                // Send IDENTIFY_PACKET command instead
                this->WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                System::pit->Sleep(10);
            }

            if(this->ReadRegister(channel, IDE_REG_ALTSTATUS) == 0xFF)
                continue; // Not connected

            ////////////////////
            // We assume valid drive if we get here
            ////////////////////

            // Read Identification Space of the Device:
            uint16_t identifyBuffer[256];
            MemoryOperations::memset(identifyBuffer, 0, sizeof(identifyBuffer));
            for(int i = 0; i < 256; i++)
                identifyBuffer[i] = inportw(this->channels[channel].commandReg + IDE_REG_DATA);

            // Construct IDE-Device structure
            IDEDevice* dev = new IDEDevice();
            MemoryOperations::memset(dev, 0, sizeof(IDEDevice));

            // Set vars
            dev->Type = type;
            dev->Channel = channel;
            dev->Drive = device;

            // Extract information from Identify Packet
            dev->specs.use48_Bit = identifyBuffer[ATA_IDENT_COMMANDSETS] & (1<<10);
            dev->specs.IO_Ready = identifyBuffer[ATA_IDENT_CAPABILITIES] & (1<<11);
            dev->specs.dmaLevel = -1;

            // Check if Master IDE capability bit is set
            if(this->pciDevice->programmingInterfaceID & (1<<7)) 
            {
                // Check for UDMA support
                uint16_t dmaSupportedBits = 0;
                if(identifyBuffer[53] & (1<<2)) // Bits in word 88 are valid
                    dmaSupportedBits = identifyBuffer[88] & 0b111111;

                // Look for highest supported UDMA
                for (dev->specs.dmaLevel = 5; dev->specs.dmaLevel >= 0; --dev->specs.dmaLevel) {
                    if (dmaSupportedBits & (1 << dev->specs.dmaLevel))
                        break;
                }

                // Check for MWDMA
                if (dev->specs.dmaLevel < 0) {
                    dmaSupportedBits = identifyBuffer[63] & 0b111;

                    for (dev->specs.dmaLevel = 2; dev->specs.dmaLevel >= 0; --dev->specs.dmaLevel) {
                        if (dmaSupportedBits & (1 << dev->specs.dmaLevel))
                            break;
                    }

                    // Fall back to old MWDMA
                    if (dev->specs.dmaLevel >= 0)
                        dev->specs.legacyDMA = true;
                }
            }

            if(IDE_DEV_DMA(dev)) // We need to setup DMA structres for this device
            {
                // Wait for busy to be cleared first
                if(!this->WaitForClear(channel, IDE_REG_ALTSTATUS, IDE_SR_BSY, IDE_TIMEOUT))
                    continue; // Hard to believe drive is not valid here, but still ignore it

                if (dev->specs.legacyDMA) {
                    // Enable MWDMA
                    Log(Info, "Enabling MWDMA level %d for drive %d:%d", dev->specs.dmaLevel, channel, device);
                    this->SetDeviceFeature(channel, ATA_FEATURE_TRANSFER_MODE, (1<<5) | dev->specs.dmaLevel);
                } else {
                    // Enable UDMA
                    Log(Info, "Enabling UDMA level %d for drive %d:%d", dev->specs.dmaLevel, channel, device);
                    this->SetDeviceFeature(channel, ATA_FEATURE_TRANSFER_MODE, (1<<6) | dev->specs.dmaLevel);
                }

                // Setup DMA Stuff
                dev->prdt = (IDEPhysRegionDescriptor*)KernelHeap::alignedMalloc(sizeof(IDEPhysRegionDescriptor), 32, &dev->prdtPhys);
                MemoryOperations::memset(dev->prdt, 0, sizeof(IDEPhysRegionDescriptor));
                dev->prdtBuffer = (uint8_t*)KernelHeap::alignedMalloc(PAGE_SIZE, PAGE_SIZE, &dev->prdtBufferPhys);
                MemoryOperations::memset(dev->prdtBuffer, 0, PAGE_SIZE);

                // Setup PRDT
                dev->prdt->bufferPtrPhys = dev->prdtBufferPhys;
                dev->prdt->byteCount = ATA_SECTOR_SIZE;
                dev->prdt->flags = (1<<15);
            }

            // Get Size for ATA drive
            // TODO: Find method for ATAPI
            if(type == IDE_ATA) {
                if (dev->specs.use48_Bit)
                    dev->Size = (uint32_t) *( (uint64_t*) &identifyBuffer[ATA_IDENT_MAX_LBA_EXT] );
                else
                    dev->Size = *( (uint32_t*) &identifyBuffer[ATA_IDENT_MAX_LBA] );
            }
            else
                dev->Size = 0;
    
            // String indicates model of device
            uint8_t* strPtr = (uint8_t*)identifyBuffer;
            for(int k = 0; k < 40; k += 2) {
                dev->Model[k] = strPtr[ATA_IDENT_MODEL + k + 1];
                dev->Model[k + 1] = strPtr[ATA_IDENT_MODEL + k];
            }
            dev->Model[40] = 0; // Terminate String.

            // Enable interrupts again
            this->SetChannelInterruptEnable(channel, true);

            // Finally add device to list
            this->devices.push_back(dev);
        }
    }

    // Print Summary of found devices
    for (int i = 0; i < this->devices.size(); i++) {  
        BootConsole::Write("Found "); BootConsole::Write(this->devices[i]->Type == 0 ? (char*)"ATA" : (char*)"ATAPI");
        BootConsole::Write(" Drive "); BootConsole::Write(Convert::IntToString(this->devices[i]->Size / 1024 / 2));
        BootConsole::Write(" MB - "); BootConsole::Write((char*)this->devices[i]->Model);
        BootConsole::WriteLine(IDE_DEV_DMA(this->devices[i]) ? (char*)" (DMA Enabled)" : (char*)"");

        // Create new system disk
        uint32_t sectSize = this->devices[i]->Type == 0 ? ATA_SECTOR_SIZE : ATAPI_SECTOR_SIZE;
        Disk* disk = new Disk(i, this, this->devices[i]->Type == 0 ? HardDisk : CDROM, (uint64_t)(this->devices[i]->Size / 2U) * (uint64_t)1024, this->devices[i]->Size / 2 / sectSize, sectSize);
        
        // Create Identifier
        int strLen = 40;
        while(this->devices[i]->Model[strLen - 1] == ' ' && strLen > 1)
            strLen--;
        
        // Allocate memory for identifier
        disk->identifier = new char[strLen + 1];

        // And copy string
        MemoryOperations::memcpy(disk->identifier, this->devices[i]->Model, strLen);
        disk->identifier[strLen] = '\0';
        
        // Finally add disk to system
        System::diskManager->AddDisk(disk);
    }

    return true;
}

char IDEController::ATA_DMA_TransferSector(uint16_t drive, uint32_t lba, uint8_t* buf, bool read)
{
    IDEDevice* dev = this->devices[drive];
    dev->prdt->byteCount = ATA_SECTOR_SIZE;

    // Write data to DMA buffer
    if(!read)
        MemoryOperations::memcpy(dev->prdtBuffer, buf, ATA_SECTOR_SIZE);

    // Reset DMA command register
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, 0);

    // Set PDRT Pointer
    outportl(this->channels[dev->Channel].bmideReg + 4, dev->prdtPhys);

    // Enable interrupts
    this->SetChannelInterruptEnable(dev->Channel, true);

    if(lba > 0xFFFFFFF) // We need to use 48-Bit addressing for this LBA
    {
        if(!dev->specs.use48_Bit)
            return 1; // 48-Bit addressing needs to be supported by drive

        // Select Drive
        this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, 0xE0 | (dev->Drive << 4));

        // Setup registers for LBA address and sector count
        this->SetCountAndLBA(dev->Channel, 1, lba, true);

        if(read)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_READ_DMA_EXT); // Send command
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_WRITE_DMA_EXT); // Send command
    }
    else // Just use 28-Bit addressing
    {
        // Select Drive
        this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, 0xE0 | (dev->Drive << 4) | (lba & 0xF000000) >> 24);
    
        // Setup registers for LBA address and sector count
        this->SetCountAndLBA(dev->Channel, 1, lba, false);

        if(read)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_READ_DMA); // Send command
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_WRITE_DMA); // Send command
    }

    // Start DMA operation by setting the right bits in the Command Register
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, (!read << 3) | (1<<0));

    // Wait for IRQ to happen, this is the true power of DMA because we can other stuff meanwhile
    this->WaitForIRQ();

    // Reset DMA Command register
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, 0);

    // Read status registers to check for errors
    uint8_t ctrlStatus = this->ReadRegister(dev->Channel, IDE_REG_BMI_STS);
    uint8_t devStatus = this->ReadRegister(dev->Channel, IDE_REG_STATUS);

    // Reset status bits in Bus Master Status
    this->WriteRegister(dev->Channel, IDE_REG_BMI_STS, ctrlStatus);

    if(ctrlStatus & (1<<1) || devStatus & (1<<0))
        return 1; // Error occurred

    if(read)
        MemoryOperations::memcpy(buf, dev->prdtBuffer, ATA_SECTOR_SIZE);
    else
    {
        // In the case of a write we also need to send a Cache flush command
        this->SetCountAndLBA(dev->Channel, 0, 0, false);
        this->WriteRegister(dev->Channel, IDE_REG_FEATURES, 0);

        if(lba > 0xFFFFFFF)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_CACHE_FLUSH);

        // Wait for IRQ and ignore errors
        this->WaitForIRQ();
    }

    return 0;
}

char IDEController::ATAPI_DMA_TransferSector(uint16_t drive, uint16_t lba, uint8_t* buf)
{
    IDEDevice* dev = this->devices[drive];
    dev->prdt->byteCount = ATAPI_SECTOR_SIZE;

    // Set PDRT Pointer
    outportl(this->channels[dev->Channel].bmideReg + 4, dev->prdtPhys);

    // Set DMA transfer direction (READ)
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, (1<<3));

    // Reset status bits in Bus Master Status
    this->WriteRegister(dev->Channel, IDE_REG_BMI_STS, this->ReadRegister(dev->Channel, IDE_REG_BMI_STS) | (1<<2) | (1<<1));

    // Select drive
    this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, dev->Drive << 4);

    // Wait for select
    this->Wait400NS(dev->Channel);

    // Send packet to device
    if(!this->SendPacketCommand(dev->Channel, ATAPI_CMD_READ, lba, 1, true, dev->specs.IO_Ready))
        return 1;

    // Start DMA operation by setting the right bits in the Command Register
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, (1<<3) | (1<<0));

    // Wait for DMA operation to complete by waiting for interrupt
    this->WaitForIRQ();

    // Read status registers to check for errors
    uint8_t ctrlStatus = this->ReadRegister(dev->Channel, IDE_REG_BMI_STS);
    uint8_t devStatus = this->ReadRegister(dev->Channel, IDE_REG_STATUS);

    // Reset DMA Command register (Stop DMA)
    this->WriteRegister(dev->Channel, IDE_REG_BMI_CMD, 0);

    // Wait for active to be cleared
    if(!this->WaitForClear(dev->Channel, IDE_REG_BMI_STS, (1<<0), IDE_TIMEOUT, true))
        return 1;

    if(ctrlStatus & (1<<1) || devStatus & (1<<0))
        return 1; // Error occurred

    MemoryOperations::memcpy(buf, dev->prdtBuffer, ATAPI_SECTOR_SIZE);

    return 0;
}

char IDEController::ATA_PIO_TransferSector(uint16_t drive, uint32_t lba, uint8_t* buf, bool read)
{
    IDEDevice* dev = this->devices[drive];

    // Disable interrupts
    this->SetChannelInterruptEnable(dev->Channel, false);

    // Wait for busy to be cleared
    if(!this->WaitForClear(dev->Channel, IDE_REG_ALTSTATUS, IDE_SR_BSY, IDE_TIMEOUT))
        return 1;

    if(lba > 0xFFFFFFF) // We need to use 48-Bit addressing for this LBA
    {
        if(!dev->specs.use48_Bit)
            return 1; // 48-Bit addressing needs to be supported by drive

        // Select Drive
        this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, 0xE0 | (dev->Drive << 4));

        // Setup registers for LBA address and sector count
        this->SetCountAndLBA(dev->Channel, 1, lba, true);

        if(read)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_READ_PIO_EXT); // Send command
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT); // Send command
    }
    else // Just use 28-Bit addressing
    {
        // Select Drive
        this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, 0xE0 | (dev->Drive << 4) | (lba & 0xF000000) >> 24);
    
        // Setup registers for LBA address and sector count
        this->SetCountAndLBA(dev->Channel, 1, lba, false);

        if(read)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_READ_PIO); // Send command
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_WRITE_PIO); // Send command
    }

    if(this->Polling(dev->Channel, true) == false)
        return 1;
    
    // Transfer data through data port
    if(read)
        this->PIOReadData(dev->Channel, dev->specs.IO_Ready, buf, ATA_SECTOR_SIZE);
    else {
        this->PIOWriteData(dev->Channel, dev->specs.IO_Ready, buf, ATA_SECTOR_SIZE);

        // In the case of a write we also need to send a Cache flush command
        if(lba > 0xFFFFFFF)
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);
        else
            this->WriteRegister(dev->Channel, IDE_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        
        // Poll again and ignore any possible errors
        if(this->Polling(dev->Channel, false) == false)
            return 1;
    }

    // All should have gone fine if we get here
    return 0;
}

char IDEController::ATAPI_PIO_TransferSector(uint16_t drive, uint16_t lba, uint8_t* buf)
{
    IDEDevice* dev = this->devices[drive];

    // Select drive
    this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, dev->Drive << 4);

    // Wait for select
    this->Wait400NS(dev->Channel);

    // Send packet to device
    if(!this->SendPacketCommand(dev->Channel, ATAPI_CMD_READ, lba, 1, false, dev->specs.IO_Ready))
        return 1;

    // Wait for IRQ
    this->WaitForIRQ();

    // Check for errors by polling (could also check status register but this works fine)
    if(!this->Polling(dev->Channel, true))
        return 1;

    // Finally read data
    this->PIOReadData(dev->Channel, dev->specs.IO_Ready, buf, ATAPI_SECTOR_SIZE);

    return 0;
}


char IDEController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    // Prevent multiple processes from using this function at the same time
    this->ideLock.Lock();

    uint8_t returnCode = 1;
    if(this->devices[drive]->Type == IDE_ATA) 
    {
        if(IDE_DEV_DMA(this->devices[drive]))
            returnCode = this->ATA_DMA_TransferSector(drive, lba, buf, true);
        else
            returnCode = this->ATA_PIO_TransferSector(drive, lba, buf, true);
    }
    else if (this->devices[drive]->Type == IDE_ATAPI)
    {
        if(IDE_DEV_DMA(this->devices[drive]))
            returnCode = this->ATAPI_DMA_TransferSector(drive, lba, buf);
        else
            returnCode = this->ATAPI_PIO_TransferSector(drive, lba, buf);
    }

    // Everything is processed so an other process can have access to this function
    this->ideLock.Unlock();

    return returnCode;
}

char IDEController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    // Prevent multiple processes from using this function at the same time
    this->ideLock.Lock();

    uint8_t returnCode = 1;
    if(this->devices[drive]->Type == IDE_ATA) 
    {
        if(IDE_DEV_DMA(this->devices[drive]))
            returnCode = this->ATA_DMA_TransferSector(drive, lba, buf, false);
        else
            returnCode = this->ATA_PIO_TransferSector(drive, lba, buf, false);
    }
    else if (this->devices[drive]->Type == IDE_ATAPI)
        Log(Error, "IDEController ATAPI::WriteSector not supported!");

    // Everything is processed so an other process can have access to this function
    this->ideLock.Unlock();

    return returnCode;
}

bool IDEController::EjectDrive(uint8_t drive)
{
    IDEDevice* dev = this->devices[drive];

    if(dev->Type != IDE_ATAPI)
        return false;

    // Select drive
    this->WriteRegister(dev->Channel, IDE_REG_HDDEVSEL, dev->Drive << 4);

    // Wait for select
    this->Wait400NS(dev->Channel);

    // Send packet to device
    if(!this->SendPacketCommand(dev->Channel, ATAPI_CMD_EJECT, 0, 0, false, dev->specs.IO_Ready))
        return 1;

    // Wait for IRQ
    this->WaitForIRQ();

    // Check for errors by polling (could also check status register but this works fine)
    if(!this->Polling(dev->Channel, false))
        return false;

    return true;
}