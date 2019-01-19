#include <system/tasking/thread.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

Thread* Thread::Create(void (*entryPoint)(), bool kernel)
{
    Thread* result = new Thread();

    result->cpuState = (CPUState*)(result->stack + 4096 - sizeof(CPUState));
    
    result->cpuState->EAX = 0;
    result->cpuState->EBX = 0;
    result->cpuState->ECX = 0;
    result->cpuState->EDX = 0;

    result->cpuState->ESI = 0;
    result->cpuState->EDI = 0;
    result->cpuState->EBP = 0;
    
    result->cpuState->EIP = (uint32_t)entryPoint;
    result->cpuState->CS = kernel ? KERNEL_CODE : USER_CODE;
	result->cpuState->DS = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuState->ES = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuState->FS = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuState->GS = kernel ? KERNEL_DATA : USER_DATA;

	result->cpuState->EFLAGS = 0x202;

	return result;
}