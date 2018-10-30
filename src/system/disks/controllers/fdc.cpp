#include <system/disks/controllers/fdc.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

void printf(char*);

void FloppyDiskController::DetectDrives()
{
    outportb(0x70, 0x10);
    unsigned drives = inportb(0x71);

    printf(" - Floppy drive 0: "); printf(drive_types[drives >> 4]); printf("\n");
    printf(" - Floppy drive 1: "); printf(drive_types[drives & 0xf]); printf("\n");

    if((drives >> 4) != 0)
    {
        FloppyDisks[0].Present = true;
        FloppyDisks[0].type = drives >> 4;
    }
    if((drives & 0xF) != 0)
    {
        FloppyDisks[1].Present = true;
        FloppyDisks[1].type = drives & 0xF;
    }
}

uint8_t FloppyDiskController::GetStatus()
{
    return inportb(FLPYDSK_MSR);
}
void FloppyDiskController::WriteDOR(uint8_t cmd)
{
    outportb(FLPYDSK_DOR, cmd);
}
void FloppyDiskController::WriteCMD(uint8_t cmd)
{
    uint8_t timeout = 0xff;
	while(--timeout) {
		if(GetStatus() & FLPYDSK_MSR_MASK_DATAREG) {
			outportb(FLPYDSK_FIFO, cmd);
			return;
		}
    } 
}
uint8_t FloppyDiskController::ReadData()
{
    for(int i = 0; i < 500; i++)
		if(GetStatus() & FLPYDSK_MSR_MASK_DATAREG)
			return inportb(FLPYDSK_FIFO);
    return 0;
}
void FloppyDiskController::WriteCCR(uint8_t cmd)
{
    outportb(FLPYDSK_CTRL, cmd);
}
void FloppyDiskController::DisableController()
{
    WriteDOR(0);
}
void FloppyDiskController::EnableController()
{
    WriteDOR(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}
void FloppyDiskController::InitDMA(uint8_t* buffer, uint32_t length)
{
    union {
		uint8_t byte[4];
		unsigned long l;
	}a, c;
	a.l = (unsigned) buffer;
	c.l = (unsigned) length -1;

	DMAController::reset (1);
	DMAController::mask_channel( FDC_DMA_CHANNEL );//Mask channel 2
	DMAController::reset_flipflop ( 1 );//Flipflop reset on DMA 1

	DMAController::set_address( FDC_DMA_CHANNEL, a.byte[0],a.byte[1]);//Buffer address
	DMAController::reset_flipflop( 1 );//Flipflop reset on DMA 1

	DMAController::set_count( FDC_DMA_CHANNEL, c.byte[0],c.byte[1]);//Set count
	DMAController::set_read ( FDC_DMA_CHANNEL );

    DMAController::unmask_all( 1 );//Unmask channel 2
}
void FloppyDiskController::CheckInt(uint32_t* st0, uint32_t* cy1)
{
    WriteCMD(FDC_CMD_CHECK_INT);
	*st0 = ReadData();
    *cy1 = ReadData();
}
uint8_t FloppyDiskController::WaitForIRQ()
{
	uint32_t timeout = pitTimer->Ticks() + 1000; //1 sec timeout

	while(floppy_irq == 0 && (pitTimer->Ticks() < timeout) );

	if(floppy_irq == 0) //timeout
		return 0;

	floppy_irq = 0;
    return 1;
}
void FloppyDiskController::SetMotor(uint8_t drive, uint8_t status)
{
    if(drive > 3)
	{
		printf("Invalid drive selected.\n");
		return;
	}
	uint8_t motor = 0;
	switch(drive)
	{
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
	if(status) {
		WriteDOR(drive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
	}
	else {
		WriteDOR(FLPYDSK_DOR_MASK_RESET);
    }

	this->pitTimer->Sleep(20);
}
void FloppyDiskController::DriveSet(uint8_t step, uint8_t loadt, uint8_t unloadt, uint8_t dma)
{
    WriteCMD(FDC_CMD_SPECIFY);
	uint8_t data = 0;
	data = ((step&0xf)<<4) | (unloadt & 0xf);
	WriteCMD(data);
	data = ((loadt << 1) | (dma?0:1));
    WriteCMD(data);
}
void FloppyDiskController::Calibrate(uint8_t drive)
{
    uint32_t st0, cy1;
	
	SetMotor(drive, 1);
	for(int i = 0; i < 10; i++)
	{
		WriteCMD(FDC_CMD_CALIBRATE);
		WriteCMD(drive);
		if(!WaitForIRQ())
		{
			goto fail;
		}
		CheckInt(&st0, &cy1);
		if(!cy1)
		{
			SetMotor(drive, 0);
			printf("Calibration successful!\n");
			return;
		}
	}
fail: SetMotor(drive, 0);
    printf("Unable to calibrate!\n");   
}
uint8_t FloppyDiskController::Seek(uint8_t cyl, uint8_t head)
{
    uint32_t st0, cy10;

	for(int i = 0;i < 10; i++)
	{
		WriteCMD(FDC_CMD_SEEK);
		WriteCMD((head<<2) | 0);
		WriteCMD(cyl);

		if(!WaitForIRQ())
		{
			printf("FATAL: IRQ link offline!\n");
			return 1;
		}
		CheckInt(&st0, &cy10);
		if(cy10 == cyl)
			return 0;
	}
    return 1;
}
uint8_t FloppyDiskController::flpy_read_sector(uint8_t head, uint8_t track, uint8_t sector)
{
    uint32_t st0, cy1;

	InitDMA((uint8_t *)DMA_BUFFER, 512);
    DMAController::set_read(FDC_DMA_CHANNEL);
	WriteCMD(FDC_CMD_READ_SECT|FDC_CMD_EXT_MULTITRACK|FDC_CMD_EXT_SKIP|FDC_CMD_EXT_DENSITY);
	WriteCMD((head << 2)| 0);
	WriteCMD(track);
	WriteCMD(head);
	WriteCMD(sector);
	WriteCMD(FLPYDSK_SECTOR_DTL_512);
	WriteCMD( ((sector+1)>=FLPY_SECTORS_PER_TRACK)?FLPY_SECTORS_PER_TRACK:(sector+1));
	WriteCMD(FLPYDSK_GAP3_LENGTH_3_5);
	WriteCMD(0xff);

	if(!WaitForIRQ())
	{
		printf("FATAL: IRQ link offline! \n");
		return 1;
	}
	for(int j = 0; j < 7; j++)
		ReadData();

	CheckInt(&st0, &cy1);
    return 1;
}
void FloppyDiskController::LbaToChs(int lba, int* head, int* track, int* sector)
{
    *head = (lba % (FLPY_SECTORS_PER_TRACK * 2) ) / FLPY_SECTORS_PER_TRACK;
	*track = lba / (FLPY_SECTORS_PER_TRACK * 2);
    *sector = lba % FLPY_SECTORS_PER_TRACK + 1;
}
uint8_t* FloppyDiskController::ReadLBA(int lba)
{
    int head = 0, track = 0, sector = 1;
	int rc = 0;
	LbaToChs(lba,  &head, &track, &sector);

	SetMotor(0, 1);
	rc = Seek(track, head);
	if(rc) {
		printf("Failed to seek!\n");
		SetMotor(0, 0);
		return 0;
	}

	flpy_read_sector(head, track, sector);
	SetMotor(0, 0);

    return (uint8_t *)DMA_BUFFER;
}
uint8_t FloppyDiskController::WriteLBA(uint8_t* buf, uint32_t lba)
{
    int head = 0, track = 0, sector = 1;
	LbaToChs(lba, &head, &track, &sector);
	SetMotor(0, 1);
	int rc = Seek(track, head);
	if(rc)
	{
		printf("Failed to seek for write!\n");
		return 1;
	}

    MemoryOperations::memcpy((uint8_t *)DMA_BUFFER, buf, 512);

	uint32_t st0, cy1;

	InitDMA((uint8_t *)DMA_BUFFER, 512);
    DMAController::set_write(FDC_DMA_CHANNEL);
	WriteCMD(FDC_CMD_WRITE_SECT|FDC_CMD_EXT_MULTITRACK|FDC_CMD_EXT_SKIP|FDC_CMD_EXT_DENSITY);
	WriteCMD((head << 2)| 0);
	WriteCMD(track);
	WriteCMD(head);
	WriteCMD(sector);
	WriteCMD(FLPYDSK_SECTOR_DTL_512);
	WriteCMD( ((sector+1)>=FLPY_SECTORS_PER_TRACK)?FLPY_SECTORS_PER_TRACK:(sector+1));
	WriteCMD(FLPYDSK_GAP3_LENGTH_3_5);
	WriteCMD(0xff);

	if(!WaitForIRQ())
	{
		printf("FATAL: IRQ link offline!\n");
		return 1;
	}
	for(int j = 0; j < 7; j++)
		ReadData();

	CheckInt(&st0, &cy1);
    return 0;
}
void FloppyDiskController::Reset()
{
    DisableController();
	EnableController();
	if(!WaitForIRQ())
	{
		printf("FATAL: IRQ link offline!\n");
		return;
	}
	printf("IRQ link is online.\n");
	uint32_t st0, cy1;
	for(int i = 0; i < 4; i++)
		CheckInt(&st0, &cy1);

	printf("Setting transfer speed to 500Kb/s\n");
	WriteCCR(0);

	Calibrate(0);

	DriveSet(3, 16, 240, 1);
}

/*///////////////////////
Public methods
*////////////////////////


FloppyDiskController::FloppyDiskController(InterruptManager* interrupts, PIT* pit)
: InterruptHandler(interrupts, interrupts->HardwareInterruptOffset() + 6)
{
	this->controllerID = "Floppy";
	this->pitTimer = pit;
}
uint32_t FloppyDiskController::HandleInterrupt(uint32_t esp)
{
    this->floppy_irq = 1;
    return esp;
}
void FloppyDiskController::InitController()
{
    printf("Searching for floppy's\n");
    DetectDrives();
    if(FloppyDisks[0].Present == false && FloppyDisks[1].Present == false)
    {
        printf("Could not find any floppy's\n");
        return;
    }
    printf("Resetting\n");
    Reset();
    printf("Floppy is ready\n");
}
char FloppyDiskController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buffer)
{
	//printf("Reading with floppy ");
	uint8_t *buf = ReadLBA(lba);
	if(!buf)
	{
		//printf(" [Error]\n");
		return 1;
	}
    MemoryOperations::memcpy(buffer, buf, 512);
	//printf(" [Done]\n");

    return 0;
}
char FloppyDiskController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    return WriteLBA(buf, lba);
}

void FloppyDiskController::AsignDisks(DiskManager* manager)
{
	if(FloppyDisks[0].Present)
	{
		Disk* disk = new Disk(0, this, DiskType::Floppy);
		manager->allDisks[manager->numDisks++] = disk;
	}
	/*
	if(FloppyDisks[1].Present)
	{
		Disk* disk = new Disk(1, this);
		manager->allDisks[manager->numDisks++] = disk;
	}
	*/ //We can not read from disk 2 yet so we disable it
}