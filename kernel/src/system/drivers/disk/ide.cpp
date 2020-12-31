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
}

uint8_t IDEController::ReadRegister(uint8_t channel, uint8_t reg)
{
    uint8_t result = 0;

    //if (reg > 0x07 && reg < 0x0C)
    //    WriteRegister(channel, IDE_REG_CONTROL, (1<<7)); // Set HOB Bit
    
    if (reg < 0x08)
        result = inportb(channels[channel].commandReg + reg);
    else if (reg < 0x0C)
        result = inportb(channels[channel].commandReg + reg - 0x06);
    else if (reg < 0x0E)
        result = inportb(channels[channel].controlReg + reg - 0x0C);
    
    //if (reg > 0x07 && reg < 0x0C)
    //    WriteRegister(channel, IDE_REG_CONTROL, 0); // Clear HOB Bit
    
    return result;
}

void IDEController::WriteRegister(uint8_t channel, uint8_t reg, uint8_t data)
{
    //if (reg > 0x07 && reg < 0x0C)
    //  WriteRegister(channel, IDE_REG_CONTROL, (1<<7)); // Set HOB Bit
    
    if (reg < 0x08)
        outportb(channels[channel].commandReg + reg, data);
    else if (reg < 0x0C)
        outportb(channels[channel].commandReg + reg - 0x06, data);
    else if (reg < 0x0E)
        outportb(channels[channel].controlReg + reg - 0x0C, data);

    //if (reg > 0x07 && reg < 0x0C)
    //    WriteRegister(channel, IDE_REG_CONTROL, 0); // Clear HOB Bit
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
    auto BAR5 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 5);

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

    Log(Info, "IDE Registers -> (%x,%x) (%x,%x) (%b, %b)",
            this->channels[IDE_CHANNEL_PRIMARY].commandReg,
            this->channels[IDE_CHANNEL_PRIMARY].controlReg,
            this->channels[IDE_CHANNEL_SECONDARY].commandReg,
            this->channels[IDE_CHANNEL_SECONDARY].controlReg,
            this->ReadRegister(IDE_CHANNEL_PRIMARY, IDE_REG_CONTROL),
            this->ReadRegister(IDE_CHANNEL_SECONDARY, IDE_REG_CONTROL));

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
            // Type of this device
            uint8_t type = IDE_ATA; 

            // Select Drive:
            WriteRegister(channel, IDE_REG_HDDEVSEL, 0xA0 | (device << 4));
            System::pit->Sleep(1);

            if(ReadRegister(channel, IDE_REG_ALTSTATUS) == 0xFF)
                continue; // Not connected

            // Set unused variables to 0
            WriteRegister(channel, IDE_REG_SECCOUNT0, 0);
            WriteRegister(channel, IDE_REG_LBA0, 0);
            WriteRegister(channel, IDE_REG_LBA1, 0);
            WriteRegister(channel, IDE_REG_LBA2, 0);
    
            // Send ATA Identify Command:
            WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_IDENTIFY);
            System::pit->Sleep(10);
    
            // Check if device exists
            if (ReadRegister(channel, IDE_REG_ALTSTATUS) == 0)
                continue; // Go to next drive

            bool canBeATAPI = false;
            while(1) {
                uint8_t status = ReadRegister(channel, IDE_REG_ALTSTATUS);
                if (status & IDE_SR_ERR) { // If Err, Device is not ATA.
                    canBeATAPI = true;
                    break;
                } 
                if (!(status & IDE_SR_BSY) && (status & IDE_SR_DRQ)) 
                    break; // Everything is right.
            }
    
            // Check for ATAPI Devices:
            if (canBeATAPI) {
                uint8_t cl = ReadRegister(channel, IDE_REG_LBA1);
                uint8_t ch = ReadRegister(channel, IDE_REG_LBA2);
    
                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).
    
                WriteRegister(channel, IDE_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                System::pit->Sleep(10);
            }

            if(ReadRegister(channel, IDE_REG_ALTSTATUS) == 0xFF)
                continue; // Not connected

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
            dev->CommandSets = *( (uint32_t*) &identifyBuffer[ATA_IDENT_COMMANDSETS] );

            // Get Size
            if (dev->CommandSets & (1 << 10))
                // Device uses 48-Bit Addressing:
                dev->Size = *( (uint32_t*) &identifyBuffer[ATA_IDENT_MAX_LBA_EXT] );
            else
                // Device uses CHS or 28-bit Addressing:
                dev->Size = *( (uint32_t*) &identifyBuffer[ATA_IDENT_MAX_LBA] );
    
            // String indicates model of device
            uint8_t* strPtr = (uint8_t*)identifyBuffer;
            for(int k = 0; k < 40; k += 2) {
                dev->Model[k] = strPtr[ATA_IDENT_MODEL + k + 1];
                dev->Model[k + 1] = strPtr[ATA_IDENT_MODEL + k];
            }
            dev->Model[40] = 0; // Terminate String.

            // Finally add device to list
            this->devices.push_back(dev);
        }
    }

    // Print Summary of found devices
    for (int i = 0; i < this->devices.size(); i++) {  
        BootConsole::Write("Found "); BootConsole::Write(this->devices[i]->Type == 0 ? (char*)"ATA" : (char*)"ATAPI");
        BootConsole::Write(" Drive "); BootConsole::Write(Convert::IntToString(this->devices[i]->Size / 1024 / 2));
        BootConsole::Write(" MB - "); BootConsole::WriteLine((char*)this->devices[i]->Model);

        // Create new system disk
        uint32_t sectSize = this->devices[i]->Type == 0 ? 512 : 2048;
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
void IDEController::HandleIRQ(uint32_t esp)
{
    uint8_t status1 = this->ReadRegister(IDE_CHANNEL_PRIMARY, IDE_REG_STATUS);
    uint8_t status2 = this->ReadRegister(IDE_CHANNEL_SECONDARY, IDE_REG_STATUS);

    Log(Info, "IDE Interrupt (%b, %b)", status1, status2);
}

char IDEController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    return 1;
}

char IDEController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    return 1;
}

bool IDEController::EjectDrive(uint8_t drive)
{
   return false;
}