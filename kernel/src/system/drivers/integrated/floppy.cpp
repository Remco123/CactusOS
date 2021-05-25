#include <system/drivers/integrated/floppy.h>
#include <core/port.h>
#include <system/memory/deviceheap.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;
using namespace CactusOS::core;

static const char* FloppyTypes[8] = {
    "none",
    "360kB 5.25\"",
    "1.2MB 5.25\"",
    "720kB 3.5\"",

    "1.44MB 3.5\"",
    "2.88MB 3.5\"",
    "unknown type",
    "unknown type"
};

FloppyDriver::FloppyDriver()
: InterruptHandler(IDT_INTERRUPT_OFFSET + 0x6), Driver("Floppy Driver", "Controls the floppy drives present on the system"), DiskController()
{
    this->IrqHasFired = false;
}

bool FloppyDriver::Initialize()
{
	// Check if there even is a floppy present
	outportb(0x70, 0x10);
    uint8_t drives = inportb(0x71);

	this->flpy1 = drives >> 4;
	this->flpy2 = drives & 0xF;
	bool present = false;
	if(flpy1 != 0)
		{Log(Info, "%s in slot 1", FloppyTypes[flpy1]); present = true;}
	if(flpy2 != 0)
		{Log(Info, "%s in slot 2", FloppyTypes[flpy2]); present = true;}
	
	if(!present) {
		Log(Info, "No Floppies installed on this controller");
		return false; // Why should we initialize when there are no floppies?
	}

    // Use address below 1MB for buffer, should be possible until 16MB but this does not seem to work
    this->bufferVirt = (uint8_t*)DeviceHeap::AllocateChunk(PAGE_SIZE);
	this->bufferPhys = 0x8000;
	VirtualMemoryManager::mapVirtualToPhysical((void*)this->bufferPhys, this->bufferVirt, true, true);
	MemoryOperations::memset(this->bufferVirt, 0, BYTES_PER_SECT);

	//Log(Info, "Using floppy buffer at: virt -> %x  phys -> %x", (uint32_t)this->bufferVirt, this->bufferPhys);

    //! reset the fdc
	ResetController();

	//! set drive information
	ConfigureDrive(13, 1, 0xf, true);

	// Assign disks
	if(this->flpy1) {
		Disk* disk1 = new Disk(0, this, DiskType::Floppy, 1_MB + 440_KB, (1_MB + 440_KB) / 512, 512);
		disk1->identifier = "Floppy 1";
		System::diskManager->AddDisk(disk1);
	}
	if(this->flpy2) {
		Disk* disk2 = new Disk(1, this, DiskType::Floppy, 1_MB + 440_KB, (1_MB + 440_KB) / 512, 512);
		disk2->identifier = "Floppy 2";
		System::diskManager->AddDisk(disk2);
	}

    return true;
}

bool FloppyDriver::InitializeDMA(FloppyDirection dir)
{
    union {
        uint8_t byte[4];//Lo[0], Mid[1], Hi[2]
        unsigned long l;
    } a, c;

    a.l = (unsigned)this->bufferPhys;
    c.l = (unsigned)BYTES_PER_SECT-1;

 	// check that address is at most 24-bits (under 16MB)
    // check that count is at most 16-bits (DMA limit)
    // check that if we add count and address we don't get a carry
    // (DMA can't deal with such a carry, this is the 64k boundary limit)
    if ((a.l >> 24) || (c.l >> 16) || (((a.l & 0xFFFF)+c.l) >> 16)){
        return false;
    }

    System::dma->Reset(1);
    System::dma->MaskChannel(FDC_DMA_CHANNEL); //Mask channel 2
    System::dma->ResetFlipFlop(1); //Flipflop reset on DMA 1

    System::dma->SetChannelAddress(FDC_DMA_CHANNEL, a.byte[0],a.byte[1]); //Buffer address
	//System::dma->SetExternalPageRegister(FDC_DMA_CHANNEL, a.byte[2]);

    System::dma->ResetFlipFlop(1); //Flipflop reset on DMA 1

    System::dma->SetChannelCounter(FDC_DMA_CHANNEL, c.byte[0],c.byte[1]); //Set count

	if(dir == FloppyDirectionWrite)
		System::dma->ChannelPrepareWrite(FDC_DMA_CHANNEL);
	else
    	System::dma->ChannelPrepareRead(FDC_DMA_CHANNEL);

    System::dma->UnmaskAll(1); //Unmask channel 2

    return true;
}

uint32_t FloppyDriver::HandleInterrupt(uint32_t esp)
{
    this->IrqHasFired = true;
    return esp;
}

void FloppyDriver::WaitForIRQ()
{
	while (this->IrqHasFired == false)
        asm("pause");
    
	this->IrqHasFired = false;
}

void FloppyDriver::WriteDOR(uint8_t val)
{
    outportb(FLPYDSK_DOR, val);
}

void FloppyDriver::WriteCCR(uint8_t val) 
{
	//! write the configuation control
	outportb(FLPYDSK_CTRL, val);
}

uint8_t FloppyDriver::ReadStatus()
{
    return inportb(FLPYDSK_MSR);
}

void FloppyDriver::SendCommand(uint8_t cmd) 
{
	//! wait until data register is ready. We send commands to the data register
	for (int i = 0; i < 500; i++ )
		if (ReadStatus() & FLPYDSK_MSR_MASK_DATAREG)
			return outportb(FLPYDSK_FIFO, cmd);
}
 
uint8_t FloppyDriver::ReadData() 
{
	//! same as above function but returns data register for reading
	for (int i = 0; i < 500; i++ )
		if (ReadStatus() & FLPYDSK_MSR_MASK_DATAREG)
			return inportb(FLPYDSK_FIFO);
	
	return inportb(FLPYDSK_FIFO);
}

//! check interrupt status command
void FloppyDriver::CheckIntStatus(uint32_t* st0, uint32_t* cyl)
{
	SendCommand(FDC_CMD_CHECK_INT);

	*st0 = ReadData();
	*cyl = ReadData();
}

//! turns the current floppy drives motor on/off
void FloppyDriver::ControlMotor(uint8_t drive, bool on)
{
	uint8_t motor = 0;

	//! select the correct mask based on current drive
	switch (drive) {

		case 0:
			motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR;
			break;
	}

	//! turn on or off the motor of that drive
	if (on)
		WriteDOR(uint8_t(drive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA));
	else
		WriteDOR(FLPYDSK_DOR_MASK_RESET);

	//! in all cases; wait a little bit for the motor to spin up/turn off
	System::pit->Sleep(20);
}
//! configure drive
void FloppyDriver::ConfigureDrive(uint8_t stepr, uint8_t loadt, uint8_t unloadt, bool dma) 
{
	uint8_t data = 0;

	//! send command
	SendCommand(FDC_CMD_SPECIFY);

	data = ( (stepr & 0xf) << 4) | (unloadt & 0xf);
	SendCommand(data);

	data = (( loadt << 1 ) | ( (dma) ? 0 : 1 ) );
	SendCommand(data);
}

//! calibrates the drive
int FloppyDriver::Calibrate(uint8_t drive) 
{
	uint32_t st0, cyl;

	if (drive >= 4)
		return -2;

	//! turn on the motor
	ControlMotor(drive, true);

	for (int i = 0; i < 10; i++) {

		//! send command
		SendCommand(FDC_CMD_CALIBRATE);
		SendCommand(drive);
		WaitForIRQ();
		CheckIntStatus(&st0, &cyl);

		//! did we find cylinder 0? if so, we are done
		if (!cyl) {
			ControlMotor(drive, false);
			return 0;
		}
	}

	ControlMotor(drive, false);
	return -1;
}

//! seek to given track/cylinder
int FloppyDriver::Seek(uint8_t drive, uint8_t cyl, uint8_t head) 
{
	uint32_t st0, cyl0;

	for (int i = 0; i < 10; i++ ) {

		//! send the command
		SendCommand(FDC_CMD_SEEK);
		SendCommand((head) << 2 | drive);
		SendCommand(cyl);

		//! wait for the results phase IRQ
		WaitForIRQ();
		CheckIntStatus(&st0, &cyl0);

		//! found the cylinder?
		if (cyl0 == cyl)
			return 0;
	}

	return -1;
}

//! disable controller
void FloppyDriver::DisableController() 
{
	WriteDOR(0);
}

//! enable controller
void FloppyDriver::EnableController() 
{
	WriteDOR(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}

//! reset controller
void FloppyDriver::ResetController()
{
	uint32_t st0, cyl;

	//! reset the controller
	DisableController();
	EnableController();
	WaitForIRQ();

	//! send CHECK_INT/SENSE INTERRUPT command to all drives
	for (int i=0; i < 4; i++)
		CheckIntStatus(&st0,&cyl);

	//! transfer speed 500kb/s
	WriteCCR(0);

	//! pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
	ConfigureDrive(3, 16, 240, true);

	//! calibrate the disks
	if(this->flpy1)
		Calibrate(0);
	if(this->flpy2)
		Calibrate(1);
}

int FloppyDriver::TransferSectorCHS(uint8_t drive, FloppyDirection dir, uint8_t head, uint8_t track, uint8_t sector)
{ 
	//! set the DMA for read transfer
	InitializeDMA(dir);

	System::pit->Sleep(100);
 
	//! read in a sector
	SendCommand((dir == FloppyDirectionRead ? FDC_CMD_READ_SECT : FDC_CMD_WRITE_SECT) | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	SendCommand(head << 2 | drive);
	SendCommand(track);
	SendCommand(head);
	SendCommand(sector);
	SendCommand(FLPYDSK_SECTOR_DTL_512);
	SendCommand(((sector + 1) >= FLPY_SECTORS_PER_TRACK) ? FLPY_SECTORS_PER_TRACK : sector + 1);
	SendCommand(FLPYDSK_GAP3_LENGTH_3_5);
	SendCommand(0xff);
 
	//! wait for irq
	WaitForIRQ();
 
	// first read status information
    unsigned char st0, st1, st2, bps;
    st0 = ReadData();
    st1 = ReadData();
    st2 = ReadData();

    // bytes per sector, should be what we programmed in
    bps = ReadData();

	//! let FDC know we handled interrupt
	//CheckIntStatus(&s_st0, &s_cyl);

    int error = 0;

    if(st0 & 0xC0) {
        static const char * status[] =
        { 0, "error", "invalid command", "drive not ready" };
        Log(Error, "Floppy Transfer Error: status = %s", status[st0 >> 6]);
        error = 1;
    }
    if(st1 & 0x80) {
        Log(Error, "Floppy Transfer Error: end of cylinder");
        error = 1;
    }
    if(st0 & 0x08) {
        Log(Error, "Floppy Transfer Error: drive not ready");
        error = 1;
    }
    if(st1 & 0x20) {
        Log(Error, "Floppy Transfer Error: CRC error");
        error = 1;
    }
    if(st1 & 0x10) {
        Log(Error, "Floppy Transfer Error: controller timeout");
        error = 1;
    }
    if(st1 & 0x04) {
        Log(Error, "Floppy Transfer Error: no data found");
        error = 1;
    }
    if((st1|st2) & 0x01) {
        Log(Error, "Floppy Transfer Error: no address mark found");
        error = 1;
    }
    if(st2 & 0x40) {
        Log(Error, "Floppy Transfer Error: deleted address mark");
        error = 1;
    }
    if(st2 & 0x20) {
        Log(Error, "Floppy Transfer Error: CRC error in data");
        error = 1;
    }
    if(st2 & 0x10) {
        Log(Error, "Floppy Transfer Error: wrong cylinder");
        error = 1;
    }
    if(st2 & 0x04) {
        Log(Error, "Floppy Transfer Error: uPD765 sector not found");
        error = 1;
    }
    if(st2 & 0x02) {
        Log(Error, "Floppy Transfer Error: bad cylinder");
        error = 1;
    }
    if(bps != 0x2) {
        Log(Error, "Floppy Transfer Error: wanted 512B/sector, got %d", (1<<(bps+7)));
        error = 1;
    }
    if(st1 & 0x02) {
        Log(Error, "Floppy Transfer Error: not writable");
        error = 2;
    }

    if(!error) {
        ControlMotor(drive, false);
        return 0;
    }
    if(error > 1) {
        Log(Error, "Floppy Transfer Error: not retrying..");
        ControlMotor(drive, false);
        return -2;
    }

	ControlMotor(drive, false);
	return 0;
}

//! convert LBA to CHS
void FloppyLBAToCHS(int lba,int *head,int *track,int *sector)
{
   *head = (lba % ( FLPY_SECTORS_PER_TRACK * 2 ) ) / (FLPY_SECTORS_PER_TRACK);
   *track = lba / ( FLPY_SECTORS_PER_TRACK * 2 );
   *sector = lba % FLPY_SECTORS_PER_TRACK + 1;
}






///////////
// Disk Functions
///////////

char FloppyDriver::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    if (drive >= 2)
		return 1;

	//! convert LBA sector to CHS
	int head = 0, track = 0, sector = 1;
	FloppyLBAToCHS(lba, &head, &track, &sector);

	//! turn motor on and seek to track
	ControlMotor(drive, true);
	if (Seek(drive, (uint8_t)track, (uint8_t)head) != 0)
		return 1;

	//! read sector and turn motor off
	int ret = TransferSectorCHS(drive, FloppyDirectionRead, (uint8_t)head, (uint8_t)track, (uint8_t)sector);
	ControlMotor(drive, false);

	MemoryOperations::memcpy(buf, this->bufferVirt, BYTES_PER_SECT);
	return ret;
}
char FloppyDriver::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    if (drive >= 2)
		return 1;

	//! convert LBA sector to CHS
	int head = 0, track = 0, sector = 1;
	FloppyLBAToCHS(lba, &head, &track, &sector);

	//! turn motor on and seek to track
	ControlMotor(drive, true);
	if (Seek(drive, (uint8_t)track, (uint8_t)head) != 0)
		return 1;
	
	// Copy buffer to DMA address
	MemoryOperations::memcpy(this->bufferVirt, buf, BYTES_PER_SECT);

	//! write sector and turn motor off
	int ret = TransferSectorCHS(drive, FloppyDirectionWrite, (uint8_t)head, (uint8_t)track, (uint8_t)sector);
	ControlMotor(drive, false);

	return ret;
}
bool FloppyDriver::EjectDrive(uint8_t drive)
{
	return false;
}