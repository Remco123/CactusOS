#include <system/tasking/procces.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

Thread* Thread::Create(void (*entryPoint)(), bool kernel)
{
    Thread* result = new Thread();

    result->kernelStack = (uint8_t*)KernelHeap::allignedMalloc(4096, 4096);
    
    result->cpuRegisters.ESP = (uint32_t)result->kernelStack + 4096;
    result->cpuRegisters.EAX = 0;
    result->cpuRegisters.EBX = 0;
    result->cpuRegisters.ECX = 0;
    result->cpuRegisters.EDX = 0;

    result->cpuRegisters.ESI = 0;
    result->cpuRegisters.EDI = 0;
    result->cpuRegisters.EBP = 0;
    
    result->cpuRegisters.EIP = (uint32_t)entryPoint;
    result->cpuRegisters.CS = kernel ? KERNEL_CODE : USER_CODE;
	result->cpuRegisters.DS = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuRegisters.ES = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuRegisters.FS = kernel ? KERNEL_DATA : USER_DATA;
	result->cpuRegisters.GS = kernel ? KERNEL_DATA : USER_DATA;

	result->cpuRegisters.EFLAGS = 0x202;

    return result;
}