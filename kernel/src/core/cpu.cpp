#include <core/cpu.h>
#include <system/bootconsole.h>

using namespace CactusOS;
using namespace CactusOS::core;
using namespace CactusOS::common;
using namespace CactusOS::system;

extern "C" void EnableSSE();

static inline void cpuid(uint32_t reg, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile("cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "0" (reg));
}

void CPU::PrintVendor()
{
    uint32_t largestStandardFunc;
    char vendor[13];
    cpuid(0, &largestStandardFunc, (uint32_t*)(vendor + 0), (uint32_t *)(vendor + 8), (uint32_t *)(vendor + 4));
    vendor[12] = '\0';

    BootConsole::Write("CPU Vendor: "); BootConsole::WriteLine(vendor);
}

void CPU::EnableFeatures()
{
    uint32_t eax, ebx, ecx, edx;

    cpuid(0x01, &eax, &ebx, &ecx, &edx);

    if (edx & EDX_SSE2) { 
        BootConsole::WriteLine("CPU Has SSE2");

        EnableSSE();
    }
    else
    {
        BootConsole::WriteLine("Error: CPU has no SSE2. This is needed");

        while(1);
    }

    if(edx & EDX_FXSR) {
        BootConsole::WriteLine("CPU Has FXSR");
    }
    else
    {
        BootConsole::WriteLine("Error: CPU has no FXSR. This is needed");

        while(1);
    } 
}