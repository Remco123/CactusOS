#include <system/drivers/disk/ahci/ahcicontroller.h>
#include <system/drivers/disk/ahci/ahcidefs.h>
#include <system/system.h>
#include <system/tasking/scheduler.h>
#include <system/tasking/lock.h>
#include <system/memory/deviceheap.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS::system::drivers;


uint32_t AHCIController::readRegister(uint32_t offset)
{
    return readMemReg(this->regBase + offset);
}
void AHCIController::writeRegister(uint32_t offset, uint32_t value)
{
    writeMemReg(this->regBase + offset, value);
}
bool AHCIController::waitForClear(uint32_t reg, uint32_t bits, uint32_t timeout)
{
	while (timeout--) {
		if ((readRegister(reg) & bits) == 0)
			return true;
		System::pit->Sleep(1);
	}
	return false;
}
bool AHCIController::waitForSet(uint32_t reg, uint32_t bits, uint32_t timeout)
{
	while (timeout--) {
		if ((readRegister(reg) & bits) == bits)
			return true;
		System::pit->Sleep(1);
	}
	return false;
}

AHCIController::AHCIController(PCIDevice* device)
: Driver("PCI AHCI Controller", "PCI AHCI Controller"),
  InterruptHandler(IDT_INTERRUPT_OFFSET + device->interrupt),
  DiskController()
{
    this->pciDevice = device;
    MemoryOperations::memset(this->ports, 0, sizeof(this->ports));
}

bool AHCIController::Initialize()
{
    BaseAddressRegister BAR5 = System::pci->GetBaseAddressRegister(pciDevice->bus, pciDevice->device, pciDevice->function, 5);
    if(BAR5.type == InputOutput)
        return false; // Should be memory mapped

    uint32_t memStart = pageRoundDown((uint32_t)BAR5.address); // Assuming 32-Bit address
    uint32_t memEnd = pageRoundUp((uint32_t)BAR5.address + BAR5.size);
    
    // Allocate virtual chuck of memory that we can use for device
    this->regBase = DeviceHeap::AllocateChunk(memEnd - memStart) + ((uint32_t)BAR5.address % PAGE_SIZE);

    // Map memory so that we can use it
    VirtualMemoryManager::mapVirtualToPhysical((void*)memStart, (void*)this->regBase, memEnd - memStart, true, true);

    // Enable BUS Mastering
    System::pci->Write(pciDevice->bus, pciDevice->device, pciDevice->function, 0x04, 0x0006);

    // Disable interrupts for now
    writeRegister(AHCI_REG_GLOBALCONTROL, readRegister(AHCI_REG_GLOBALCONTROL) & ~(1<<1));

    // Try to reset controller into default state
    if(this->Reset() == false)
        return false;

    // Read amount of ports from register
	this->portCount = 1 + ((readRegister(AHCI_REG_HOSTCAP) >> CAP_NP_SHIFT) & CAP_NP_MASK);
    uint32_t portsImplemented = readRegister(AHCI_REG_PORTIMPLEMENTED);

    // For each implemented port we create a new class for handling port specific things
	for (int i = 0; i < this->portCount; i++) {
		if (portsImplemented & (1 << i)) {
			this->ports[i] = new AHCIPort(this, this->regBase + AHCI_REG_PORTBASE + (i * AHCI_PORTREG_SIZE), i);
			if(this->ports[i]->PreparePort() == false) {
				delete this->ports[i];
				this->ports[i] = 0;
			}
		}
	}

    // Clear interrupt status
    writeRegister(AHCI_REG_INTSTATUS, readRegister(AHCI_REG_INTSTATUS));

    // Enable interrupts
    writeRegister(AHCI_REG_GLOBALCONTROL, readRegister(AHCI_REG_GLOBALCONTROL) | (1<<1));
    
    // Enable all found ports
	for (int i = 0; i < this->portCount; i++) {
		if (this->ports[i]) {
			if(this->ports[i]->StartupPort() == false) {
				delete this->ports[i];
				this->ports[i] = 0;
			}
		}
	}

    return true;
}

bool AHCIController::Reset()
{
    // Store usefull variables before reset
    uint32_t capsBeforeReset = readRegister(AHCI_REG_HOSTCAP) & (CAP_SMPS | CAP_SSS | CAP_SPM | CAP_EMS | CAP_SXS);
	uint32_t piBeforeReset = readRegister(AHCI_REG_PORTIMPLEMENTED);

    // First set AHCI Enable bit
    writeRegister(AHCI_REG_GLOBALCONTROL, readRegister(AHCI_REG_GLOBALCONTROL) | (1<<31));

    // Then set the HBA Reset bit
    writeRegister(AHCI_REG_GLOBALCONTROL, readRegister(AHCI_REG_GLOBALCONTROL) | (1<<0));

    // Wait for reset completion
    if(!waitForClear(AHCI_REG_GLOBALCONTROL, (1<<0), 1000))
        return false;
    
    // Set AHCI Enable bit again
    writeRegister(AHCI_REG_GLOBALCONTROL, readRegister(AHCI_REG_GLOBALCONTROL) | (1<<31));

    // Write back stored variables
    writeRegister(AHCI_REG_HOSTCAP, readRegister(AHCI_REG_HOSTCAP) | capsBeforeReset);
    writeRegister(AHCI_REG_PORTIMPLEMENTED, piBeforeReset);

    return true;
}

uint32_t AHCIController::HandleInterrupt(uint32_t esp)
{
    uint32_t interruptPending = readRegister(AHCI_REG_INTSTATUS) & readRegister(AHCI_REG_PORTIMPLEMENTED);

	if (interruptPending == 0)
		return esp;

	for (int i = 0; i < this->portCount; i++) {
		if (interruptPending & (1 << i)) {
			if (this->ports[i])
				this->ports[i]->HandleExternalInterrupt();
		}
	}

	// clear pending interrupts
	writeRegister(AHCI_REG_INTSTATUS, interruptPending);

    //Log(Info, "AHCI Interrupt!");
    return esp;
}







char AHCIController::ReadSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    return this->ports[drive]->TransferData(true, lba, buf, 1) ? 0 : 1;
}
char AHCIController::WriteSector(uint16_t drive, uint32_t lba, uint8_t* buf)
{
    return this->ports[drive]->TransferData(false, lba, buf, 1) ? 0 : 1;
}
bool AHCIController::EjectDrive(uint8_t drive)
{
    return this->ports[drive]->Eject() ? 0 : 1;
}