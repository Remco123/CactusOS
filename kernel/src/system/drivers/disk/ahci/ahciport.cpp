#include <system/drivers/disk/ahci/ahcicontroller.h>
#include <system/drivers/disk/ahci/ahcidefs.h>
#include <system/drivers/disk/ahci/ahciport.h>
#include <system/system.h>
#include <system/tasking/scheduler.h>
#include <system/tasking/lock.h>
#include <system/memory/deviceheap.h>
#include <system/drivers/disk/ide.h>
#include <system/disks/disk.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;

uint32_t AHCIPort::readRegister(uint32_t offset)
{
    return readMemReg(this->portBase + offset);
}
void AHCIPort::writeRegister(uint32_t offset, uint32_t value)
{
    writeMemReg(this->portBase + offset, value);
}
bool AHCIPort::waitForClear(uint32_t reg, uint32_t bits, uint32_t timeout)
{
	while (timeout--) {
		if ((readRegister(reg) & bits) == 0)
			return true;
		System::pit->Sleep(1);
	}
	return false;
}
bool AHCIPort::waitForSet(uint32_t reg, uint32_t bits, uint32_t timeout)
{
	while (timeout--) {
		if ((readRegister(reg) & bits) == bits)
			return true;
		System::pit->Sleep(1);
	}
	return false;
}
int AHCIPort::FindFreeCMDSlot()
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (readRegister(AHCI_PORTREG_SATAACTIVE) | readRegister(AHCI_PORTREG_COMMANDISSUE));
	for (int i = 0; i < COMMAND_LIST_COUNT; i++)
	{
		if ((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	return -1;
}

AHCIPort::AHCIPort(AHCIController* parent, uint32_t regBase, int index)
{
    this->parent = parent;
    this->portBase = regBase;
	this->index = index;
}

AHCIPort::~AHCIPort()
{
    if(this->commandList)
        delete this->commandList;
    
    if(this->fis)
        delete this->fis;
}

bool AHCIPort::PreparePort()
{
	if(this->Disable() == false)
		return false;

    // Allocate required structures for this port
    this->commandList = (a_commandHeader_t*)KernelHeap::alignedMalloc(sizeof(a_commandHeader_t) * COMMAND_LIST_COUNT, 1_KB, &this->commandListPhys);
    this->fis = (a_fis_t*)KernelHeap::alignedMalloc(sizeof(a_fis_t), 256, &this->fisPhys);

    // Clear out the allocated memory
    MemoryOperations::memset(this->commandList, 0, sizeof(a_commandHeader_t) * COMMAND_LIST_COUNT);
    MemoryOperations::memset(this->fis, 0, sizeof(a_fis_t));

    // Update port registers to point to structures
    writeRegister(AHCI_PORTREG_CMDLISTBASE, this->commandListPhys);
    writeRegister(AHCI_PORTREG_CMDLISTBASE2, 0);
    writeRegister(AHCI_PORTREG_FISLISTBASE, this->fisPhys);
    writeRegister(AHCI_PORTREG_FISLISTBASE2, 0);

    // Disable transitions to partial or slumber state
    writeRegister(AHCI_PORTREG_SATACTRL, readRegister(AHCI_PORTREG_SATACTRL) | (1<<8) | (1<<9));

	// Clear IRQ status and error bits
    writeRegister(AHCI_PORTREG_INTSTATUS, readRegister(AHCI_PORTREG_INTSTATUS));
    writeRegister(AHCI_PORTREG_SATAERROR, readRegister(AHCI_PORTREG_SATAERROR));

	// Power and Spin up device
    writeRegister(AHCI_PORTREG_CMDANDSTATUS, readRegister(AHCI_PORTREG_CMDANDSTATUS) | PORT_CMD_POD | PORT_CMD_SUD);

	// Activate link
	writeRegister(AHCI_PORTREG_CMDANDSTATUS, (readRegister(AHCI_PORTREG_CMDANDSTATUS) & ~PORT_CMD_ICC_MASK) | PORT_CMD_ICC_ACTIVE);

	// Enable FIS receive (enabled when fb set, only to be disabled when unset)
	writeRegister(AHCI_PORTREG_CMDANDSTATUS, readRegister(AHCI_PORTREG_CMDANDSTATUS) | PORT_CMD_FRE);

    return true;
}

bool AHCIPort::StartupPort()
{
	if(this->Enable() == false)
		return false;
	
	// Enable interrupts
	writeRegister(AHCI_PORTREG_INTENABLE, PORT_INT_MASK);

	uint32_t portType = this->GetType();
	//Log(Info, "AHCI: Port %d is of type %d", this->index, portType);

	if(portType != AHCI_DEV_NULL) {
		// Check if this device is a cdrom device or not
		this->isATATPI = (portType == AHCI_DEV_SATAPI);

		// Now we can send a Identify command to the device
		// Data will be the same as with the IDE controller
		uint16_t identifyBuffer[256];
		if(this->Identify((uint8_t*)identifyBuffer) == false)
			return false;

		uint32_t diskSize = 0;
		char diskModel[41];

		// Extract Command set from buffer
        uint32_t commandSet = *( (uint32_t*) &identifyBuffer[ATA_IDENT_COMMANDSETS] );
    
        // Get Size:
        if (commandSet & (1 << 26)) {
            // Device uses 48-Bit Addressing:
            diskSize = *( (uint32_t*) &identifyBuffer[ATA_IDENT_MAX_LBA_EXT] );
			this->useLBA48 = true;
		}
        else {
            // Device uses CHS or 28-bit Addressing:
            diskSize = *( (uint32_t*) &identifyBuffer[ATA_IDENT_MAX_LBA] );
			this->useLBA48 = false;
		}
    
        // String indicates model of device
        uint8_t* strPtr = (uint8_t*)identifyBuffer;
        for(int k = 0; k < 40; k += 2) {
            diskModel[k] = strPtr[ATA_IDENT_MODEL + k + 1];
            diskModel[k + 1] = strPtr[ATA_IDENT_MODEL + k];
        }
        diskModel[40] = 0; // Terminate String.

		Log(Info, "AHCI: Found %s drive %s", this->isATATPI ? "ATAPI" : "ATA", diskModel);
        uint32_t sectSize = this->isATATPI ? 2048 : 512;

		// Create disk structure
		Disk* disk = new Disk(this->index, this->parent, this->isATATPI ? CDROM : HardDisk, (uint64_t)(diskSize / 2U) * (uint64_t)1024, diskSize / 2 / sectSize, sectSize);
            
        // Create Identifier
        int strLen = 40;
        while(diskModel[strLen - 1] == ' ' && strLen > 1)
            strLen--;
        disk->identifier = new char[strLen + 1];
            
        MemoryOperations::memcpy(disk->identifier, diskModel, strLen);
        disk->identifier[strLen] = '\0';
            
        System::diskManager->AddDisk(disk);
	}

    return true;
}

uint32_t AHCIPort::GetType()
{
	uint32_t status = readRegister(AHCI_PORTREG_SATASTATUS);
 
	uint8_t speed = (status >> 8) & 0x0F;
	uint8_t devDET = status & 0x0F;
 
	if (devDET != AHCI_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (speed != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;
 
	switch (readRegister(AHCI_PORTREG_SIGNATURE))
	{
		case SATA_SIG_ATAPI:
			return AHCI_DEV_SATAPI;
		case SATA_SIG_SEMB:
			return AHCI_DEV_SEMB;
		case SATA_SIG_PM:
			return AHCI_DEV_PM;
		default:
			return AHCI_DEV_SATA;
	}	
}

bool AHCIPort::Enable()
{
	// Wait until CR (bit15) is cleared
	if (!waitForClear(AHCI_PORTREG_CMDANDSTATUS, PORT_CMD_CR, 500))
		return false;

	// Start port
	writeRegister(AHCI_PORTREG_CMDANDSTATUS, readRegister(AHCI_PORTREG_CMDANDSTATUS) | PORT_CMD_ST);
	return true;
}


bool AHCIPort::Disable()
{
	// Disable port
	writeRegister(AHCI_PORTREG_CMDANDSTATUS, readRegister(AHCI_PORTREG_CMDANDSTATUS) & ~PORT_CMD_ST);

	// Wait until bit is cleared
	if (!waitForClear(AHCI_PORTREG_CMDANDSTATUS, PORT_CMD_CR, 500))
		return false;

	return true;
}

void AHCIPort::HandleExternalInterrupt()
{
	uint32_t status = readRegister(AHCI_PORTREG_INTSTATUS);
	//Log(Info, "AHCIPort: Interrupt %x", status);
	writeRegister(AHCI_PORTREG_INTSTATUS, status); // Clear interrupts
}

bool AHCIPort::Identify(uint8_t* buffer)
{
	int slot = this->FindFreeCMDSlot();
	if (slot == -1)
		return false;

	uint32_t bufPhys = 0;
	uint8_t* buf = (uint8_t*)KernelHeap::alignedMalloc(512, 16, &bufPhys);
	MemoryOperations::memset(buf, 0, 512);
 
	a_commandHeader_t* cmdheader = &this->commandList[slot];
	cmdheader->flags = (sizeof(FIS_REG_H2D) / sizeof(uint32_t)) | (0<<6) | (1<<16);

	uint32_t cmdTablePhys = 0;
	a_commandTable_t* cmdTable = (a_commandTable_t*)KernelHeap::alignedMalloc(sizeof(a_commandTable_t), 128, &cmdTablePhys);
	MemoryOperations::memset(cmdTable, 0, sizeof(a_commandTable_t));

	// Make header point to allocated table
	cmdheader->cmdTableAddress = cmdTablePhys;
	cmdheader->cmdTableAddressHigh = 0;

	// Setup PRDT Entry
	cmdTable->prdt_entry[0].dataBase = bufPhys;
	cmdTable->prdt_entry[0].byteCount = 511;
	cmdTable->prdt_entry[0].ioc = 1;
 
	// Setup command
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdTable->fis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = this->isATATPI ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;

	if(!waitForClear(AHCI_PORTREG_TASKFILE, 0x80 | 0x8, 1000)) {
		Log(Error, "AHCI: Port is stuck!");
		
		KernelHeap::allignedFree(buf);
		KernelHeap::allignedFree(cmdTable);
		cmdheader->cmdTableAddress = 0;
		return false;
	}
	
	// Enable command slot
	writeRegister(AHCI_PORTREG_COMMANDISSUE, 1<<slot);

	bool ret = true;
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<slot)) == 0) 
			break;
		
		if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30)) { // Task file error
			ret = false;
			break;
		}
	}
 
	// Check again
	if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30))
		ret = false;
	
	if(ret)
		MemoryOperations::memcpy(buffer, buf, 512);
	
	KernelHeap::allignedFree(buf);
	KernelHeap::allignedFree(cmdTable);
	cmdheader->cmdTableAddress = 0;

	return ret;
}

bool AHCIPort::TransferData(bool dirIn, uint32_t lba, uint8_t* buffer, uint32_t count)
{
	int slot = this->FindFreeCMDSlot();
	if (slot == -1)
		return false;

	// Required buffer size
	uint32_t size = count * (this->isATATPI ? 2048 : 512);
	uint32_t entryCount = (size / 8_KB + 1); // 8K bytes (16 sectors) per PRDT
	uint32_t i = 0;
	uint32_t count2 = count;

	uint32_t bufPhys = 0;
	uint8_t* buf = (uint8_t*)KernelHeap::alignedMalloc(size, 16, &bufPhys);
	MemoryOperations::memset(buf, 0, size);
 
	a_commandHeader_t* cmdheader = &this->commandList[slot];
	cmdheader->flags = (sizeof(FIS_REG_H2D) / sizeof(uint32_t)) | (0<<6) | (entryCount<<16);

	uint32_t cmdTablePhys = 0;
	a_commandTable_t* cmdTable = (a_commandTable_t*)KernelHeap::alignedMalloc(sizeof(a_commandTable_t) + (entryCount-1) * sizeof(a_prdtEntry_t), 128, &cmdTablePhys);
	MemoryOperations::memset(cmdTable, 0, sizeof(a_commandTable_t) + (entryCount-1) * sizeof(a_prdtEntry_t));

	// Make header point to allocated table
	cmdheader->cmdTableAddress = cmdTablePhys;
	cmdheader->cmdTableAddressHigh = 0;

	// Setup required entries
	for (i = 0; i < (entryCount-1); i++)
	{
		cmdTable->prdt_entry[i].dataBase = bufPhys;
		cmdTable->prdt_entry[i].byteCount = 8*1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdTable->prdt_entry[i].ioc = 1;
		bufPhys += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdTable->prdt_entry[i].dataBase = bufPhys;
	cmdTable->prdt_entry[i].byteCount = (size % 4_KB)-1; // 512 bytes per sector
	cmdTable->prdt_entry[i].ioc = 1;

	if(this->isATATPI)
	{
		// Setup command
		FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdTable->fis);
	
		cmdfis->fis_type = FIS_TYPE_REG_H2D;
		cmdfis->c = 1;	// Command
		cmdfis->command = ATA_CMD_PACKET;
		cmdfis->featurel = 1;
		cmdfis->featureh = 0;
		cmdfis->countl = count2 & 0xFF;
		cmdfis->counth = (count2 >> 8) & 0xFF;

		cmdheader->flags |= (1<<5); // Set A Bit in header flags

		cmdTable->cmd[ 0] = ATAPI_CMD_READ;
		cmdTable->cmd[ 1] = 0x0;
		cmdTable->cmd[ 2] = (lba >> 24) & 0xFF;
		cmdTable->cmd[ 3] = (lba >> 16) & 0xFF;
		cmdTable->cmd[ 4] = (lba >> 8) & 0xFF;
		cmdTable->cmd[ 5] = (lba >> 0) & 0xFF;
		cmdTable->cmd[ 6] = 0x0;
		cmdTable->cmd[ 7] = 0x0;
		cmdTable->cmd[ 8] = 0x0;
		cmdTable->cmd[ 9] = count2;
		cmdTable->cmd[10] = 0x0;
		cmdTable->cmd[11] = 0x0;
	}
	else
	{
		// Setup command
		FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdTable->fis);
	
		cmdfis->fis_type = FIS_TYPE_REG_H2D;
		cmdfis->c = 1;	// Command
		cmdfis->command = dirIn ? (this->useLBA48 ? ATA_CMD_READ_DMA_EXT : ATA_CMD_READ_DMA) : (this->useLBA48 ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_WRITE_DMA);
	
		cmdfis->lba0 = (uint8_t)lba;
		cmdfis->lba1 = (uint8_t)(lba>>8);
		cmdfis->lba2 = (uint8_t)(lba>>16);
		cmdfis->device = 1<<6;	// LBA mode
	
		cmdfis->lba3 = (uint8_t)(lba>>24);
		cmdfis->lba4 = 0; // TODO: Use when we might ever use 64-bit lba
		cmdfis->lba5 = 0; // Not used for now
	
		cmdfis->countl = count2 & 0xFF;
		cmdfis->counth = (count2 >> 8) & 0xFF;	
	}

	if(!waitForClear(AHCI_PORTREG_TASKFILE, 0x80 | 0x8, 1000)) {
		Log(Error, "AHCI: Port is stuck!");
		
		KernelHeap::allignedFree(buf);
		KernelHeap::allignedFree(cmdTable);
		cmdheader->cmdTableAddress = 0;
		return false;
	}
	
	// Enable command slot
	writeRegister(AHCI_PORTREG_COMMANDISSUE, 1<<slot);

	bool ret = true;
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<slot)) == 0) 
			break;
		
		if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30)) { // Task file error
			ret = false;
			break;
		}
	}
 
	// Check again
	if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30))
		ret = false;
	
	if(ret)
		MemoryOperations::memcpy(buffer, buf, size);
	
	KernelHeap::allignedFree(buf);
	KernelHeap::allignedFree(cmdTable);
	cmdheader->cmdTableAddress = 0;

	return ret;
}			


bool AHCIPort::Eject()
{
	if(this->isATATPI == false)
		return false; // This it not going to work, no matter how hard we try :)
	
	int slot = this->FindFreeCMDSlot();
	if (slot == -1)
		return false;
 
	a_commandHeader_t* cmdheader = &this->commandList[slot];
	cmdheader->flags = (sizeof(FIS_REG_H2D) / sizeof(uint32_t)) | (0<<6) | (0<<16);

	uint32_t cmdTablePhys = 0;
	a_commandTable_t* cmdTable = (a_commandTable_t*)KernelHeap::alignedMalloc(sizeof(a_commandTable_t), 128, &cmdTablePhys);
	MemoryOperations::memset(cmdTable, 0, sizeof(a_commandTable_t));

	// Make header point to allocated table
	cmdheader->cmdTableAddress = cmdTablePhys;
	cmdheader->cmdTableAddressHigh = 0;

	// Setup command
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdTable->fis);
	
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_PACKET;
	cmdfis->featurel = 1;
	cmdfis->featureh = 0;
	cmdfis->countl = 1;

	cmdheader->flags |= (1<<5); // Set A Bit in header flags

	cmdTable->cmd[ 0] = ATAPI_CMD_EJECT;
	cmdTable->cmd[ 4] = 0x02;

	if(!waitForClear(AHCI_PORTREG_TASKFILE, 0x80 | 0x8, 1000)) {
		Log(Error, "AHCI: Port is stuck!");
		
		KernelHeap::allignedFree(cmdTable);
		cmdheader->cmdTableAddress = 0;
		return false;
	}
	
	// Enable command slot
	writeRegister(AHCI_PORTREG_COMMANDISSUE, 1<<slot);

	bool ret = true;
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<slot)) == 0) 
			break;
		
		if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30)) { // Task file error
			ret = false;
			break;
		}
	}
 
	// Check again
	if (readRegister(AHCI_PORTREG_COMMANDISSUE) & (1<<30))
		ret = false;
	
	KernelHeap::allignedFree(cmdTable);
	cmdheader->cmdTableAddress = 0;

	return ret;
}