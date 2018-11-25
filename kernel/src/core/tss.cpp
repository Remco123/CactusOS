#include <core/tss.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

static TSSEntry tss;

extern "C" void flush_tss();

void TSS::Install(uint32_t idx, uint16_t kernelSS, uint16_t kernelESP)
{
    //! install TSS descriptor
	uint32_t base = (uint32_t) &tss;

	//! install descriptor
	GlobalDescriptorTable::SetDescriptor (idx, base, base + sizeof (TSSEntry), 0xE9, 0);

	//! initialize TSS
    MemoryOperations::memset ((void*) &tss, 0, sizeof (TSSEntry));

	//! set stack and segments
	tss.ss0 = kernelSS;
	tss.esp0 = kernelESP;
	tss.cs = 0x0b;
	tss.ss = 0x13;
	tss.es = 0x13;
	tss.ds = 0x13;
	tss.fs = 0x13;
	tss.gs = 0x13;

	//! flush tss
	flush_tss();
}

void TSS::SetStack(uint16_t kernelSS, uint16_t kernelESP)
{
    tss.ss0 = kernelSS;
    tss.esp0 = kernelESP;
}