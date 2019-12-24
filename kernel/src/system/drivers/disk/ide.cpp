#include <system/drivers/disk/ide.h>
#include <system/system.h>
#include <system/tasking/scheduler.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

uint8_t ide_buf[2048] = {0};
static volatile uint8_t IRQTriggered = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

DECLARE_LOCK(IDE_LOCK);

IDEInterruptHandler::IDEInterruptHandler(IDEController* controller, uint32_t number)
: InterruptHandler(IDT_INTERRUPT_OFFSET + number)
{
    this->controller = controller;
}

uint32_t IDEInterruptHandler::HandleInterrupt(uint32_t esp)
{
    if(this->controller != 0)
        this->controller->HandleIRQ();
    return esp;
}




IDEController::IDEController(PCIDevice* device)
: Driver("PCI IDE Controller", "PCI IDE Controller"),
  DiskController()
{
    this->pciDevice = device;

    //Clear ideDevices list
    MemoryOperations::memset((void*)this->ideDevices, 0, sizeof(this->ideDevices));

    //Register Interrupt Handlers
    this->intHandle1 = new IDEInterruptHandler(this, 14);
    this->intHandle2 = new IDEInterruptHandler(this, 15);
}

void IDEController::HandleIRQ()
{
    IRQTriggered = 1;
}

void IDEController::WaitForIRQ()
{
    while(!IRQTriggered);
    IRQTriggered = 0;
}

bool IDEController::Initialize()
{
    //Get Base Address Registers
    BaseAddressRegister BAR0 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 0);
    BaseAddressRegister BAR1 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 1);
    BaseAddressRegister BAR2 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 2);
    BaseAddressRegister BAR3 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 3);
    BaseAddressRegister BAR4 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 4);

    //Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY  ].base  = BAR0.address + 0x1F0 * (!BAR0.address);
    channels[ATA_PRIMARY  ].ctrl  = BAR1.address + 0x3F6 * (!BAR1.address);
    channels[ATA_SECONDARY].base  = BAR2.address + 0x170 * (!BAR2.address);
    channels[ATA_SECONDARY].ctrl  = BAR3.address + 0x376 * (!BAR3.address);
    channels[ATA_PRIMARY  ].bmide = BAR4.address + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = BAR4.address + 8; // Bus Master IDE

    //Disable IRQs:
    WriteRegister(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    WriteRegister(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    //Detect ATA-ATAPI Devices:
    int count = 0;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
    
            uint8_t err = 0, type = IDE_ATA, status;
            ideDevices[count].Reserved = 0; // Assuming that no drive here.
    
            //Select Drive:
            WriteRegister(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            System::pit->Sleep(1); // Wait 1ms for drive select to work.
    
            //Send ATA Identify Command:
            WriteRegister(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            System::pit->Sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                    // it is based on System Timer Device Driver.
    
            //Polling:
            if (ReadRegister(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.
    
            while(1) {
                status = ReadRegister(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }
    
            //Probe for ATAPI Devices:
            if (err != 0) {
                uint8_t cl = ReadRegister(i, ATA_REG_LBA1);
                uint8_t ch = ReadRegister(i, ATA_REG_LBA2);
    
                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).
    
                WriteRegister(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                System::pit->Sleep(1);
            }
    
            //Read Identification Space of the Device:
            ReadDeviceBuffer(i, ATA_REG_DATA, (uint32_t) ide_buf, 128);
    
            //Read Device Parameters:
            ideDevices[count].Reserved     = 1;
            ideDevices[count].Type         = type;
            ideDevices[count].Channel      = i;
            ideDevices[count].Drive        = j;
            ideDevices[count].Signature    = *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
            ideDevices[count].Capabilities = *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
            ideDevices[count].CommandSets  = *((uint32_t *)(ide_buf + ATA_IDENT_COMMANDSETS));
    
            //Get Size:
            if (ideDevices[count].CommandSets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ideDevices[count].Size   = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ideDevices[count].Size   = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA));
    
            //String indicates model of device:
            for(int k = 0; k < 40; k += 2) {
                ideDevices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ideDevices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];}
            ideDevices[count].Model[40] = 0; // Terminate String.
    
            count++;
        }
    
    //Print Summary:
    for (int i = 0; i < 4; i++)
        if (ideDevices[i].Reserved == 1) {            
            BootConsole::Write("Found "); BootConsole::Write(ideDevices[i].Type == 0 ? (char*)"ATA" : (char*)"ATAPI");
            BootConsole::Write(" Drive "); BootConsole::Write(Convert::IntToString(ideDevices[i].Size / 1024 / 2));
            BootConsole::Write("MB - "); BootConsole::WriteLine((char*)ideDevices[i].Model);

            Disk* disk = new Disk(i, this);
            
            //////////
            // Create Identifier
            //////////
            int strLen = 40;
            while(ideDevices[i].Model[strLen - 1] == ' ' && strLen > 1)
                strLen--;
            disk->identifier = new char[strLen + 1];
            
            MemoryOperations::memcpy(disk->identifier, ideDevices[i].Model, strLen);
            disk->identifier[strLen] = '\0';
            
            System::diskManager->AddDisk(disk);
        }

    return true;
}

uint8_t IDEController::ReadRegister(uint8_t channel, uint8_t reg)
{
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        WriteRegister(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        result = inportb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = inportb(channels[channel].base  + reg - 0x06);
    else if (reg < 0x0E)
        result = inportb(channels[channel].ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        result = inportb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    return result;
}

void IDEController::WriteRegister(uint8_t channel, uint8_t reg, uint8_t data)
{
    if (reg > 0x07 && reg < 0x0C)
      WriteRegister(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    if (reg < 0x08)
        outportb(channels[channel].base  + reg - 0x00, data);
    else if (reg < 0x0C)
        outportb(channels[channel].base  + reg - 0x06, data);
    else if (reg < 0x0E)
        outportb(channels[channel].ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        outportb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void IDEController::ReadDeviceBuffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads)
{
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
    *           ESP for all of the code the compiler generates between the inline
    *           assembly blocks.
    */
    if (reg > 0x07 && reg < 0x0C)
        WriteRegister(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    asm("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08)
        insl(channels[channel].base  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insl(channels[channel].base  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insl(channels[channel].ctrl  + reg - 0x0A, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    asm("popw %es;");
    if (reg > 0x07 && reg < 0x0C)
        WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

uint8_t IDEController::Polling(uint8_t channel, uint32_t advanced_check)
{
    //Delay 400 nanosecond for BSY to be set:
    for(int i = 0; i < 4; i++)
        ReadRegister(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
    
    //Wait for BSY to be cleared:
    while (ReadRegister(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.
    
    if (advanced_check) {
        uint8_t state = ReadRegister(channel, ATA_REG_STATUS); // Read Status Register.
    
        //Check For Errors:
        if (state & ATA_SR_ERR)
            return 2; // Error.
    
        //Check If Device fault:
        if (state & ATA_SR_DF)
            return 1; // Device Fault.
    
        //Check DRQ:
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set  
    }
    
    return 0; // No Error.
}

uint8_t IDEController::PrintErrorCode(uint32_t drive, uint8_t err)
{
    if (err == 0) 
        return err;

    if (err == 1)
    {
        BootConsole::WriteLine("- Device Fault");
        err = 19;      
    }
    else if (err == 2) 
    {
        uint8_t st = ReadRegister(ideDevices[drive].Channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)       
        {
            BootConsole::WriteLine("- No Address Mark Found");
            err = 7;
        }
        if (st & ATA_ER_TK0NF)      
        {
            BootConsole::WriteLine("- No Media or Media Error");
            err = 3;
        }
        if (st & ATA_ER_ABRT)       
        {
            BootConsole::WriteLine("- Command Aborted");
            err = 20;
        }
        if (st & ATA_ER_MCR)        
        {
            BootConsole::WriteLine("- No Media or Media Error");
            err = 3;
        }
        if (st & ATA_ER_IDNF)       
        {
            BootConsole::WriteLine("- ID mark not Found");
            err = 21;
        }
        if (st & ATA_ER_MC)         
        {
            BootConsole::WriteLine("- No Media or Media Error");
            err = 3;
        }
        if (st & ATA_ER_UNC)        
        {
            BootConsole::WriteLine("- Uncorrectable Data Error");
            err = 22;
        }
        if (st & ATA_ER_BBK)        
        {
            BootConsole::WriteLine("- Bad Sectors");
            err = 13;
        }
    }
    else  if (err == 3) {
        BootConsole::WriteLine("- Reads Nothing");
        err = 23;
    }
    else  if (err == 4) {
        BootConsole::WriteLine("- Write Protected");
        err = 8;
    }

    return err;
}

char IDEController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    LOCK(IDE_LOCK);
    uint8_t returnCode = 0;
    
    //Check if the drive presents:
    if (drive > 3 || ideDevices[drive].Reserved == 0) returnCode = 0x1;      // Drive Not Found!
    
    //Check if inputs are valid:
    else if (((lba) > ideDevices[drive].Size) && (ideDevices[drive].Type == IDE_ATA))
        returnCode = 0x2;                     // Seeking to invalid position.
    
    //Read in PIO Mode through Polling & IRQs:
    else {
        uint8_t err;
        if (ideDevices[drive].Type == IDE_ATA)
        {
            err = AtaReadSector(drive, lba, buf);
        }
        else if (ideDevices[drive].Type == IDE_ATAPI)
        {
            err = ATAPIReadSector(drive, lba, buf);
        }
        returnCode = PrintErrorCode(drive, err);
    }
    UNLOCK(IDE_LOCK);
    return returnCode;
}

char IDEController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    LOCK(IDE_LOCK);
    uint8_t returnCode = 0;

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ideDevices[drive].Reserved == 0)
        returnCode = 0x1;      // Drive Not Found!
    // 2: Check if inputs are valid:
    // ==================================
    else if ((lba > ideDevices[drive].Size) && (ideDevices[drive].Type == IDE_ATA))
        returnCode = 0x2;                     // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        uint8_t err;
        if (ideDevices[drive].Type == IDE_ATA)
            err = AtaWriteSector(drive, lba, buf);
        else if (ideDevices[drive].Type == IDE_ATAPI)
            err = 4; // Write-Protected.
        returnCode = PrintErrorCode(drive, err);
    }
    UNLOCK(IDE_LOCK);
    return returnCode;
}

uint8_t IDEController::AtaReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    uint8_t lba_io[6];
    uint32_t channel = ideDevices[drive].Channel; // Read the Channel.
    uint32_t slavebit = ideDevices[drive].Drive; // Read the Drive [Master/Slave]
    uint32_t bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    uint32_t words = 256; // Almost every ATA drive has a sector-size of 512-byte.
    uint32_t numsects = 1;
    uint16_t cyl;
    uint8_t head, sect, err;

    WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN = (IRQTriggered = 0x0) + 0x02);

    //Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
                                // giving a wrong LBA.
        // LBA48:
        lba_mode  = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (ideDevices[drive].Capabilities & 0x200)  { // Drive supports LBA?
        // LBA28:
        lba_mode  = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head      = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode  = 0;
        sect      = (lba % 63) + 1;
        cyl       = (lba + 1  - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }
    //See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    //Wait if the drive is busy;
    while (ReadRegister(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

    //Select Drive from the controller;
    if (lba_mode == 0)
        WriteRegister(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        WriteRegister(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    //Write Parameters;
    if (lba_mode == 2) {
        WriteRegister(channel, ATA_REG_SECCOUNT1,   0);
        WriteRegister(channel, ATA_REG_LBA3,   lba_io[3]);
        WriteRegister(channel, ATA_REG_LBA4,   lba_io[4]);
        WriteRegister(channel, ATA_REG_LBA5,   lba_io[5]);
    }
    WriteRegister(channel, ATA_REG_SECCOUNT0,   numsects);
    WriteRegister(channel, ATA_REG_LBA0,   lba_io[0]);
    WriteRegister(channel, ATA_REG_LBA1,   lba_io[1]);
    WriteRegister(channel, ATA_REG_LBA2,   lba_io[2]);

    if (lba_mode == 0 && dma == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0) cmd = ATA_CMD_READ_PIO;   
    if (lba_mode == 2 && dma == 0) cmd = ATA_CMD_READ_PIO_EXT;   
    if (lba_mode == 0 && dma == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1) cmd = ATA_CMD_READ_DMA_EXT;
    WriteRegister(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma)
        return 1;
    else
    {
        // PIO Read.
        if ((err = Polling(channel, 1)))
            return err; // Polling, set error and exit if there is.
        inportsm(bus, buf, words);

        WriteRegister(channel, ATA_REG_COMMAND, (char []) {  (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
        Polling(channel, 0); // Polling.
    }
 
    return 0; // Easy, isn't it?
}

uint8_t IDEController::AtaWriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    uint8_t lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    uint8_t lba_io[6];
    uint32_t channel = ideDevices[drive].Channel; // Read the Channel.
    uint32_t slavebit = ideDevices[drive].Drive; // Read the Drive [Master/Slave]
    uint32_t bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    uint32_t words = 256; // Almost every ATA drive has a sector-size of 512-byte.
    uint32_t numsects = 1;
    uint16_t cyl;
    uint8_t head, sect, err;

    WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN = (IRQTriggered = 0x0) + 0x02);

    //Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are
                                // giving a wrong LBA.
        // LBA48:
        lba_mode  = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
        head      = 0; // Lower 4-bits of HDDEVSEL are not used here.
    } else if (ideDevices[drive].Capabilities & 0x200)  { // Drive supports LBA?
        // LBA28:
        lba_mode  = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head      = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode  = 0;
        sect      = (lba % 63) + 1;
        cyl       = (lba + 1  - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
    }
    //See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    //Wait if the drive is busy;
    while (ReadRegister(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

    //Select Drive from the controller;
    if (lba_mode == 0)
        WriteRegister(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        WriteRegister(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    //Write Parameters;
    if (lba_mode == 2) {
        WriteRegister(channel, ATA_REG_SECCOUNT1, 0);
        WriteRegister(channel, ATA_REG_LBA3, lba_io[3]);
        WriteRegister(channel, ATA_REG_LBA4, lba_io[4]);
        WriteRegister(channel, ATA_REG_LBA5, lba_io[5]);
    }
    WriteRegister(channel, ATA_REG_SECCOUNT0, numsects);
    WriteRegister(channel, ATA_REG_LBA0, lba_io[0]);
    WriteRegister(channel, ATA_REG_LBA1, lba_io[1]);
    WriteRegister(channel, ATA_REG_LBA2, lba_io[2]);

    if (lba_mode == 0 && dma == 0) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
    WriteRegister(channel, ATA_REG_COMMAND, cmd); // Send the Command.

    if (dma)
        return 1;
    else
    {
        // PIO Write.
        if ((err = Polling(channel, 0)))
            return err; // Polling, set error and exit if there is.
        outportsm(bus, buf, words);

        WriteRegister(channel, ATA_REG_COMMAND, (char []) {  (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
        Polling(channel, 0); // Polling.
    }
 
    return 0; // Easy, isn't it?
}

uint8_t IDEController::ATAPIReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    uint32_t channel  = ideDevices[drive].Channel;
    uint32_t slavebit = ideDevices[drive].Drive;
    uint32_t bus      = channels[channel].base;
    uint32_t words    = 1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
    uint32_t numsects = 1;
    uint8_t  err;
    
    // Enable IRQs:
    WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN = IRQTriggered = 0x0);

    //Setup SCSI Packet:
    atapi_packet[ 0] = ATAPI_CMD_READ;
    atapi_packet[ 1] = 0x0;
    atapi_packet[ 2] = (lba >> 24) & 0xFF;
    atapi_packet[ 3] = (lba >> 16) & 0xFF;
    atapi_packet[ 4] = (lba >> 8) & 0xFF;
    atapi_packet[ 5] = (lba >> 0) & 0xFF;
    atapi_packet[ 6] = 0x0;
    atapi_packet[ 7] = 0x0;
    atapi_packet[ 8] = 0x0;
    atapi_packet[ 9] = numsects;
    atapi_packet[10] = 0x0;
    atapi_packet[11] = 0x0;

    //Select the drive:
    WriteRegister(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    //Delay 400 nanoseconds for select to complete:
    for(int i = 0; i < 4; i++)
        ReadRegister(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns.
    
    //Inform the Controller that we use PIO mode:
    WriteRegister(channel, ATA_REG_FEATURES, 0);         // PIO mode.

    //Tell the Controller the size of buffer:
    WriteRegister(channel, ATA_REG_LBA1, (words * 2) & 0xFF);   // Lower Byte of Sector Size.
    WriteRegister(channel, ATA_REG_LBA2, (words * 2) >> 8);   // Upper Byte of Sector Size.

    //Send the Packet Command:
    WriteRegister(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.
    
    //Waiting for the driver to finish or return an error code:
    if ((err = Polling(channel, 1))) return err;         // Polling and return if error.
    
    //Sending the packet data:
    asm("rep   outsw" : : "c"(6), "d"(bus), "S"(atapi_packet));   // Send Packet Data

    //Receiving Data:
    for (unsigned int i = 0; i < numsects; i++) {
        WaitForIRQ();                  // Wait for an IRQ.
        if ((err = Polling(channel, 1)))
            return err;      // Polling and return if error.

        uint32_t readSize = ((uint32_t)ReadRegister(channel, ATA_REG_LBA2) << 8 | (uint32_t)ReadRegister(channel, ATA_REG_LBA1));
        if(readSize != words*2)
        {
            return 1;
        }

        inportsm(bus, buf, readSize/2);
    }

    //Waiting for an IRQ:
    WaitForIRQ();
    
    //Waiting for BSY & DRQ to clear:
    while (ReadRegister(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));
    
    return 0; // Easy, ... Isn't it?
}

bool IDEController::EjectDrive(uint8_t drive)
{
   uint32_t channel = ideDevices[drive].Channel;
   uint32_t slavebit = ideDevices[drive].Drive;
   uint32_t bus = channels[channel].base;
   uint8_t err = 0;
   bool returnCode;
   IRQTriggered = 0;
 
   //Check if the drive presents:
   if (drive > 3 || ideDevices[drive].Reserved == 0)
      returnCode = false;      // Drive Not Found!

   //Check if drive isn't ATAPI:
   else if (ideDevices[drive].Type == IDE_ATA)
      returnCode = false;         // Command Aborted.

   //Eject ATAPI Drive:
   else {
      // Enable IRQs:
      WriteRegister(channel, ATA_REG_CONTROL, channels[channel].nIEN = IRQTriggered = 0x0);
 
      //Setup SCSI Packet:
      atapi_packet[ 0] = ATAPI_CMD_EJECT;
      atapi_packet[ 1] = 0x00;
      atapi_packet[ 2] = 0x00;
      atapi_packet[ 3] = 0x00;
      atapi_packet[ 4] = 0x02;
      atapi_packet[ 5] = 0x00;
      atapi_packet[ 6] = 0x00;
      atapi_packet[ 7] = 0x00;
      atapi_packet[ 8] = 0x00;
      atapi_packet[ 9] = 0x00;
      atapi_packet[10] = 0x00;
      atapi_packet[11] = 0x00;
 
      //Select the Drive:
      WriteRegister(channel, ATA_REG_HDDEVSEL, slavebit << 4);
 
      //Delay 400 nanosecond for select to complete:
      for(int i = 0; i < 4; i++)
         ReadRegister(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
 
      //Send the Packet Command:
      WriteRegister(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.
 
      //Waiting for the driver to finish or invoke an error:
      err = Polling(channel, 1);            // Polling and stop if error.
 
      //Sending the packet data:
      asm("rep outsw"::"c"(6), "d"(bus), "S"(atapi_packet));// Send Packet Data
      WaitForIRQ(); // Wait for an IRQ.
      
      err = Polling(channel, 1); // Polling and get error code.
      
      if (err == 3) 
        err = 0; // DRQ is not needed here.
      
      returnCode = PrintErrorCode(drive, err);
      if(err == 0)
        returnCode = true;
   }
   return returnCode;
}