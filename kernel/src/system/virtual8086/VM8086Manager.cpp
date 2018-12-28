#include <system/virtual8086/VM86Manager.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

static const uint16_t codeSegment = 0x2000;
static const uint16_t stackSegment = 0x8000;

CPUState vm86CPUState;

extern "C" uint8_t __VM86CodeStart;
extern "C" uint8_t __VM86CodeEnd;
extern "C" uint8_t __Int86;

extern "C" uintptr_t cpuGetEIP();
extern "C" uintptr_t cpuGetESP();
// vm86
extern "C" void cpuEnterV86Int(int ignored, int arg1, int arg2, int arg3, int arg4, int arg6);
extern "C" void cpuEnterV86(uint32_t ss, uint32_t esp, uint32_t cs, uint32_t eip, uint32_t eax);

Virtual8086Manager::Virtual8086Manager()
: InterruptHandler(0xFD)
{ 
    // install int86 trampoline code in conventional memory
    uint32_t codeSize = &__VM86CodeEnd - &__VM86CodeStart;
    uint8_t* dst = (uint8_t*)(codeSegment << 4);
    uint8_t* src = &__VM86CodeStart;
    while(codeSize--) *dst++ = *src++;
}

uint32_t Virtual8086Manager::HandleInterrupt(common::uint32_t esp)
{
    CPUState* state = (CPUState*)esp;
    uintptr_t args[] = { state->EAX, state->EBX, state->ECX, state->EDX, state->ESI, state->EDI, state->EBP };
    
    InterruptDescriptorTable::DisableInterrupts();

    MemoryOperations::memcpy(&vm86CPUState, state, sizeof(CPUState));

    TSS::GetCurrent()->esp0 = cpuGetESP();
    TSS::GetCurrent()->eip = cpuGetEIP();

    //BootConsole::WriteLine("[CPU] Entering VM8086 Mode");
    cpuEnterV86(args[1], args[2], args[3], args[4], args[5]);

    return esp;
}

void vm86Enter(uint16_t ss, uint16_t sp, uint16_t cs, uint16_t ip, uint32_t arg)
{
    cpuEnterV86Int(0, ss, sp, cs, ip, arg);
}

void Virtual8086Manager::CallInterrupt(common::uint8_t intNumber, VM86Registers* regs)
{
    uint32_t size = sizeof(VM86Registers);
    uint8_t*dst = (uint8_t*)((codeSegment << 4) + 0x8000);
    uint8_t*src = (uint8_t*)regs;
    while(size--) *dst++ = *src++;
    vm86Enter(stackSegment, 0x0000, codeSegment, &__Int86 - &__VM86CodeStart, intNumber);
}