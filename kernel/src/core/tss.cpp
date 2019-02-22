#include <core/tss.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

static TSSEntry tss;

extern "C" void flush_tss();

void TSS::Install(uint32_t idx, uint32_t kernelSS, uint32_t kernelESP)
{
	MemoryOperations::memset(&tss, 0, sizeof(TSSEntry));

    //! install TSS descriptor
	uint32_t base = (uint32_t) &tss;

	//! install descriptor
	GlobalDescriptorTable::SetDescriptor (idx, base, base + sizeof (TSSEntry), 0xE9, 0);

	//! initialize TSS
    MemoryOperations::memset ((void*) &tss, 0, sizeof (TSSEntry));

	//! set stack and segments
	tss.ss0 = kernelSS;
	tss.esp0 = kernelESP;
	tss.iomap = sizeof(TSSEntry);

	//! flush tss
	flush_tss();
}

void TSS::SetStack(uint32_t kernelSS, uint32_t kernelESP)
{
    tss.ss0 = kernelSS;
    tss.esp0 = kernelESP;
}

TSSEntry* TSS::GetCurrent()
{
	return &tss;
}