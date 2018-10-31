#include <core/tss.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS;

extern "C" void tss_flush(common::uint16_t sel);

void printf(char*);

static tss_entry TSS;

void TaskStateSegment::SetStack(common::uint16_t kernelSS, common::uint16_t kernelESP)
{
    TSS.ss0 = kernelSS;
	TSS.esp0 = kernelESP;
}
void TaskStateSegment::InstallTSS(GlobalDescriptorTable* gdt, common::uint16_t kernelSS, common::uint16_t kernelESP)
{
    //! install TSS descriptor
	uint32_t base = (uint32_t) &TSS;

	//! install descriptor    
	printf("Installing TSS gdt entry with base: "); printf(Convert::IntToString(base)); printf(" and limit: "); printf(Convert::IntToString(base + sizeof(tss_entry))); printf("\n");
    gdt->tssSegmentSelector = GlobalDescriptorTable::SegmentDescriptor(base, base + sizeof(tss_entry), 0xE9);

	//! initialize TSS
 	MemoryOperations::memset ((void*) &TSS, 0, sizeof (tss_entry));

	//! set stack and segments
	TSS.ss0 = kernelSS;
	TSS.esp0 = kernelESP;
	TSS.cs=0x0b;
	TSS.ss = 0x13;
	TSS.es = 0x13;
	TSS.ds = 0x13;
	TSS.fs = 0x13;
	TSS.gs = 0x13;

	//! flush tss
	tss_flush (5 * sizeof (GlobalDescriptorTable::SegmentDescriptor));
}