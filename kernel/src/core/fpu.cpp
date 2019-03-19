#include <core/fpu.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

void FPU::Enable()
{
	uint32_t cr4;
	asm volatile ("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= 0x200;
	asm volatile ("mov %0, %%cr4" :: "r"(cr4));

    uint32_t cw = 0x37F;
    asm volatile("fldcw %0" :: "m"(cw));
}