#include <system/disks/controllers/ide.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

void printf(char*);





IDEInterruptHandler::IDEInterruptHandler(IDEController* controller, InterruptManager* ints, uint32_t number)
: InterruptHandler(ints, ints->HardwareInterruptOffset() + number)
{
    this->controller = controller;
}

uint32_t IDEInterruptHandler::HandleInterrupt(uint32_t esp)
{
    if(this->controller != 0)
        this->controller->ide_handle_irq();
    return esp;
}









IDEController::IDEController(InterruptManager* interrupts)
{
    this->intHandle1 = new IDEInterruptHandler(this, interrupts, 14);
    this->intHandle2 = new IDEInterruptHandler(this, interrupts, 15);
}

void IDEController::InitIDE(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4, core::PIT* pit)
{
    int i, j, k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY  ].base  = (BAR0 &= 0xFFFFFFFC) + 0x1F0*(!BAR0);
    channels[ATA_PRIMARY  ].ctrl  = (BAR1 &= 0xFFFFFFFC) + 0x3F4*(!BAR1);
    channels[ATA_SECONDARY].base  = (BAR2 &= 0xFFFFFFFC) + 0x170*(!BAR2);
    channels[ATA_SECONDARY].ctrl  = (BAR3 &= 0xFFFFFFFC) + 0x374*(!BAR3);
    channels[ATA_PRIMARY  ].bmide = (BAR4 &= 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (BAR4 &= 0xFFFFFFFC) + 8; // Bus Master IDE

    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
   for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++) {

        printf("Checking disk: "); printf(Convert::IntToString(i)); printf(":"); printf(Convert::IntToString(j)); printf("\n");

         unsigned char err = 0, type = IDE_ATA, status;
         ide_devices[count].Reserved   = 0; // Assuming that no drive here.

         // (I) Select Drive:
         ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
         pit->Sleep(1); // Wait 1ms for drive select to work.

         ide_write(i, ATA_REG_SECCOUNT0, 0);
         ide_write(i, ATA_REG_LBA0, 0);
         ide_write(i, ATA_REG_LBA1, 0);
         ide_write(i, ATA_REG_LBA2, 0);

         // (II) Send ATA Identify Command:
         ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
         pit->Sleep(1); // This function should be implemented in your OS. which waits for 1 ms. it is based on System Timer Device Driver.

        
         // (III) Polling:
         if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

         while(1) {
            status = ide_read(i, ATA_REG_STATUS);
            if ( (status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
            if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
         }


         // (IV) Probe for ATAPI Devices:

         if (err != 0) {
            unsigned char cl = ide_read(i, ATA_REG_LBA1);
            unsigned char ch = ide_read(i, ATA_REG_LBA2);
 
            if (cl == 0x14 && ch ==0xEB)
               type = IDE_ATAPI;
            else if (cl == 0x69 && ch == 0x96)
               type = IDE_ATAPI;
            else
               continue; // Unknown Type (may not be a device).
 
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
            pit->Sleep(1);
         }

         // (V) Read Identification Space of the Device:
         ide_read_buffer(i, ATA_REG_DATA, (unsigned int) ide_buf, 128);
 
         // (VI) Read Device Parameters:
         ide_devices[count].Reserved     = 1;
         ide_devices[count].Type         = type;
         ide_devices[count].Channel      = i;
         ide_devices[count].Drive        = j;
         ide_devices[count].Signature    = *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
         ide_devices[count].Capabilities = *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
         ide_devices[count].CommandSets  = *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));
 
         // (VII) Get Size:
         if (ide_devices[count].CommandSets & (1 << 26))
            // Device uses 48-Bit Addressing:
            ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
         else
            // Device uses CHS or 28-bit Addressing:
            ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));
 
         // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
         for(k = 0; k < 40; k += 2) {
            ide_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
            ide_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];}
         ide_devices[count].Model[40] = 0; // Terminate String.
 
         count++;
      }

    // 4- Print Summary:
    for (i = 0; i < 4; i++)
        if (ide_devices[i].Reserved == 1) {
            printf(" Found "); printf((char *[]){"ATA", "ATAPI"}[ide_devices[i].Type]); printf(" Drive "); printf(Convert::IntToString(ide_devices[i].Size/1024/1024/2));
            printf("GB - "); printf((char*)ide_devices[i].Model); printf("\n");
        }
}

unsigned char IDEController::ide_read(unsigned char channel, unsigned char reg) {
   unsigned char result;
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      result = inportb(channels[channel].base + reg - 0x00);
   else if (reg < 0x0C)
      result = inportb(channels[channel].base  + reg - 0x06);
   else if (reg < 0x0E)
      result = inportb(channels[channel].ctrl  + reg - 0x0A);
   else if (reg < 0x16)
      result = inportb(channels[channel].bmide + reg - 0x0E);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
   return result;
}

void IDEController::ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      outportb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outportb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outportb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outportb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void IDEController::ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads) {
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
    *           ESP for all of the code the compiler generates between the inline
    *           assembly blocks.
    */
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
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
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char IDEController::ide_polling(unsigned char channel, unsigned int advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    for(int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
  
    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.
  
    if (advanced_check) {
        unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.
  
        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
          return 2; // Error.
  
        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
          return 1; // Device Fault.
  
        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
          return 3; // DRQ should be set
  
    }
  
    return 0; // No Error.
}

unsigned char IDEController::ide_print_error(unsigned int drive, unsigned char err) {
   
   if (err == 0) return err;

   printf(" IDE:");
   if (err == 1) {printf("- Device Fault\n     "); err = 19;}
   else if (err == 2) {
      unsigned char st = ide_read(ide_devices[drive].Channel, ATA_REG_ERROR);
      if (st & ATA_ER_AMNF)   {printf("- No Address Mark Found\n     ");   err = 7;}
      if (st & ATA_ER_TK0NF)   {printf("- No Media or Media Error\n     ");   err = 3;}
      if (st & ATA_ER_ABRT)   {printf("- Command Aborted\n     ");      err = 20;}
      if (st & ATA_ER_MCR)   {printf("- No Media or Media Error\n     ");   err = 3;}
      if (st & ATA_ER_IDNF)   {printf("- ID mark not Found\n     ");      err = 21;}
      if (st & ATA_ER_MC)   {printf("- No Media or Media Error\n     ");   err = 3;}
      if (st & ATA_ER_UNC)   {printf("- Uncorrectable Data Error\n     ");   err = 22;}
      if (st & ATA_ER_BBK)   {printf("- Bad Sectors\n     ");       err = 13;}
   } else  if (err == 3)           {printf("- Reads Nothing\n     "); err = 23;}
     else  if (err == 4)  {printf("- Write Protected\n     "); err = 8;}
   /*
   printf("- [%s %s] %s\n",
      (const char *[]){"Primary","Secondary"}[ide_devices[drive].channel],
      (const char *[]){"Master", "Slave"}[ide_devices[drive].drive],
      ide_devices[drive].model);
    */
   return err;
}

void IDEController::ide_wait_irq()
{
    while (!ide_irq_invoked);
    ide_irq_invoked = 0;
}

void IDEController::ide_handle_irq()
{
    ide_irq_invoked = 1;
}


char IDEController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    printf("IDE: Read Sector\n");
    char returnCode = 0;
    
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].Reserved == 0) returnCode = 0x1;      // Drive Not Found!
    
    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba) > ide_devices[drive].Size) && (ide_devices[drive].Type == IDE_ATA))
        returnCode = 0x2;                     // Seeking to invalid position.
    
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (ide_devices[drive].Type == IDE_ATA)
        {
            printf("Reading with ATA\n");
            err = AtaReadSector(drive, lba, buf);
        }
        else if (ide_devices[drive].Type == IDE_ATAPI)
        {
            printf("Reading with ATAPI\n");
            err = ATAPIReadSector(drive, lba, buf);
        }
        returnCode = ide_print_error(drive, err);
    }
    return returnCode;
}
char IDEController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    char returnCode = 0;

    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].Reserved == 0)
        returnCode = 0x1;      // Drive Not Found!
    // 2: Check if inputs are valid:
    // ==================================
    else if ((lba > ide_devices[drive].Size) && (ide_devices[drive].Type == IDE_ATA))
        returnCode = 0x2;                     // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        unsigned char err;
        if (ide_devices[drive].Type == IDE_ATA)
            err = 0;//ide_ata_access(ATA_WRITE, drive, lba, numsects, es, edi);
        else if (ide_devices[drive].Type == IDE_ATAPI)
            err = 4; // Write-Protected.
        returnCode = ide_print_error(drive, err);
    }
    return returnCode;
}

unsigned char IDEController::AtaReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    unsigned char lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
    unsigned char lba_io[6];
    unsigned int  channel      = ide_devices[drive].Channel; // Read the Channel.
    unsigned int  slavebit      = ide_devices[drive].Drive; // Read the Drive [Master/Slave]
    unsigned int  bus = channels[channel].base; // Bus Base, like 0x1F0 which is also data port.
    unsigned int  words      = 256; // Almost every ATA drive has a sector-size of 512-byte.
    unsigned int numsects = 1;
    unsigned short cyl, i;
    unsigned char head, sect, err;

    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);

    // (I) Select one from LBA28, LBA48 or CHS;
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
    } else if (ide_devices[drive].Capabilities & 0x200)  { // Drive supports LBA?
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
    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

       // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters;
    if (lba_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1,   0);
        ide_write(channel, ATA_REG_LBA3,   lba_io[3]);
        ide_write(channel, ATA_REG_LBA4,   lba_io[4]);
        ide_write(channel, ATA_REG_LBA5,   lba_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0,   numsects);
    ide_write(channel, ATA_REG_LBA0,   lba_io[0]);
    ide_write(channel, ATA_REG_LBA1,   lba_io[1]);
    ide_write(channel, ATA_REG_LBA2,   lba_io[2]);

    if (lba_mode == 0 && dma == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0) cmd = ATA_CMD_READ_PIO;   
    if (lba_mode == 2 && dma == 0) cmd = ATA_CMD_READ_PIO_EXT;   
    if (lba_mode == 0 && dma == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1) cmd = ATA_CMD_READ_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma)
        printf("DMA Not supported yet");// DMA Read.
    else
    {
        // PIO Read.
        if (err = ide_polling(channel, 1))
            return err; // Polling, set error and exit if there is.
        inportsm(bus, buf, words);

        ide_write(channel, ATA_REG_COMMAND, (char []) {  (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH,
                        (char)ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
        ide_polling(channel, 0); // Polling.
    }
 
    return 0; // Easy, isn't it?
}
unsigned char IDEController::AtaWriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{

}



unsigned char IDEController::ATAPIReadSector(common::uint16_t drive, common::uint32_t lba, common::uint8_t* buf)
{
    unsigned int   channel  = ide_devices[drive].Channel;
    unsigned int   slavebit = ide_devices[drive].Drive;
    unsigned int   bus      = channels[channel].base;
    unsigned int   words    = 1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
    unsigned int   numsects = 1;
    unsigned char  err;
    int i;
    
    // Enable IRQs:
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

    // (I): Setup SCSI Packet:
    // ------------------------------------------------------------------
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

    // (II): Select the drive:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    // (III): Delay 400 nanoseconds for select to complete:
    // ------------------------------------------------------------------
    for(int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns.
    
    // (IV): Inform the Controller that we use PIO mode:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_FEATURES, 0);         // PIO mode.

    // (V): Tell the Controller the size of buffer:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);   // Lower Byte of Sector Size.
    ide_write(channel, ATA_REG_LBA2, (words * 2) >> 8);   // Upper Byte of Sector Size.

    // (VI): Send the Packet Command:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.
    
    // (VII): Waiting for the driver to finish or return an error code:
    // ------------------------------------------------------------------
    if (err = ide_polling(channel, 1)) return err;         // Polling and return if error.
    
    // (VIII): Sending the packet data:
    // ------------------------------------------------------------------
    asm("rep   outsw" : : "c"(6), "d"(bus), "S"(atapi_packet));   // Send Packet Data

    // (IX): Receiving Data:
    // ------------------------------------------------------------------
    for (i = 0; i < numsects; i++) {
        ide_wait_irq();                  // Wait for an IRQ.
        if (err = ide_polling(channel, 1))
            return err;      // Polling and return if error.
        inportsm(bus, buf, words);
    }

    // (X): Waiting for an IRQ:
    // ------------------------------------------------------------------
    ide_wait_irq();
    
    // (XI): Waiting for BSY & DRQ to clear:
    // ------------------------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
        ;
    
    return 0; // Easy, ... Isn't it?
}



char IDEController::EjectDrive(unsigned char drive) {
   unsigned int   channel      = ide_devices[drive].Channel;
   unsigned int   slavebit      = ide_devices[drive].Drive;
   unsigned int   bus      = channels[channel].base;
   unsigned int   words      = 2048 / 2;               // Sector Size in Words.
   unsigned char  err = 0;
   char returnCode;
   ide_irq_invoked = 0;
 
   // 1: Check if the drive presents:
   // ==================================
   if (drive > 3 || ide_devices[drive].Reserved == 0)
      returnCode = 0x1;      // Drive Not Found!
   // 2: Check if drive isn't ATAPI:
   // ==================================
   else if (ide_devices[drive].Type == IDE_ATA)
      returnCode = 20;         // Command Aborted.
   // 3: Eject ATAPI Driver:
   // ============================================
   else {
      // Enable IRQs:
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);
 
      // (I): Setup SCSI Packet:
      // ------------------------------------------------------------------
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
 
      // (II): Select the Drive:
      // ------------------------------------------------------------------
      ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);
 
      // (III): Delay 400 nanosecond for select to complete:
      // ------------------------------------------------------------------
      for(int i = 0; i < 4; i++)
         ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
 
      // (IV): Send the Packet Command:
      // ------------------------------------------------------------------
      ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);      // Send the Command.
 
      // (V): Waiting for the driver to finish or invoke an error:
      // ------------------------------------------------------------------
      err = ide_polling(channel, 1);            // Polling and stop if error.
 
      // (VI): Sending the packet data:
      // ------------------------------------------------------------------
      asm("rep   outsw"::"c"(6), "d"(bus), "S"(atapi_packet));// Send Packet Data
      ide_wait_irq();                  // Wait for an IRQ.
      err = ide_polling(channel, 1);            // Polling and get error code.
      if (err == 3) err = 0; // DRQ is not needed here.
      returnCode = ide_print_error(drive, err); // Return;
   }
   return returnCode;
}