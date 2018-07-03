#include <core/cpu.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;

void printf(char*);

inline void CPU::cpuid(uint32_t reg, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "0" (reg));
}

void CPU::CollectInfo()
{
    // Register storage
    uint32_t eax, ebx, ecx, edx;

    // Function 0x00 - Vendor-ID and Largest Standard Function

    uint32_t largestStandardFunc;
    char vendor[13];
    cpuid(0, &largestStandardFunc, (uint32_t*)(vendor + 0), (uint32_t *)(vendor + 8), (uint32_t *)(vendor + 4));
    vendor[12] = '\0';

    printf("CPU Vendor: "); printf(vendor); printf("\n");

    // Function 0x01 - Feature Information

    if (largestStandardFunc >= 0x01)
    {
        cpuid(0x01, &eax, &ebx, &ecx, &edx);

        printf("Features:"); //TODO: Add all instructions and features.

        if (edx & EDX_PSE)      { printf(" PSE"); PSE = true;}
        if (edx & EDX_PAE)      { printf(" PAE"); PAE = true;}
        if (edx & EDX_APIC)     { printf(" APIC"); APIC = true;}
        if (edx & EDX_MTRR)     { printf(" MTRR"); MTRR = true;}
 
        printf("\n"); 
 
        printf("Instructions:"); 
 
        if (edx & EDX_TSC)      { printf(" TSC"); TSC = true;}
        if (edx & EDX_MSR)      { printf(" MSR"); MSR = true;}
        if (edx & EDX_SSE)      { printf(" SSE"); SSE = true;}
        if (edx & EDX_SSE2)     { printf(" SSE2"); SSE2 = true;}
        if (ecx & ECX_SSE3)     { printf(" SSE3"); SSE3 = true;}
        if (ecx & ECX_SSSE3)    { printf(" SSSE3"); SSSE3 = true;}
        if (ecx & ECX_SSE41)    { printf(" SSE41"); SSE41 = true;}
        if (ecx & ECX_SSE42)    { printf(" SSE42"); SSE42 = true;}
        if (ecx & ECX_AVX)      { printf(" AVX"); AVX = true;}
        if (edx & EDX_MMX)      { printf(" MMX"); MMX = true; }
        if (ecx & ECX_F16C)     { printf(" F16C"); F16C = true;}
        if (ecx & ECX_RDRAND)   { printf(" RDRAND"); RDRAND = true;}

        printf("\n");
    }

    // Extended Function 0x00 - Largest Extended Function

    uint32_t largestExtendedFunc;
    cpuid(0x80000000, &largestExtendedFunc, &ebx, &ecx, &edx);

    // Extended Function 0x01 - Extended Feature Bits

    if (largestExtendedFunc >= 0x80000001)
    {
        cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

        if (edx & EDX_64_BIT)
        {
            printf("64-bit Architecture\n");
        }
    }

    // Extended Function 0x02-0x04 - Processor Name / Brand String

    if (largestExtendedFunc >= 0x80000004)
    {
        char name[48];
        cpuid(0x80000002, (uint32_t *)(name +  0), (uint32_t *)(name +  4), (uint32_t *)(name +  8), (uint32_t *)(name + 12));
        cpuid(0x80000003, (uint32_t *)(name + 16), (uint32_t *)(name + 20), (uint32_t *)(name + 24), (uint32_t *)(name + 28));
        cpuid(0x80000004, (uint32_t *)(name + 32), (uint32_t *)(name + 36), (uint32_t *)(name + 40), (uint32_t *)(name + 44));

        // Processor name is right justified with leading spaces
        char *p = name;
        while (*p == ' ')
        {
            ++p;
        }

        printf("CPU Name: "); printf(p); printf("\n");
    }
}

void CPU::EnableFeatures()
{
    if(this->SSE)
    {
        enable_sse();
        printf("- SSE Enabled\n");
    }
}

