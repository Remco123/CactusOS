#include <system/tasking/thread.h>

#include <system/memory/heap.h> 
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

ThreadHelper::ThreadHelper()
{   }

Thread* ThreadHelper::CreateFromFunction(void (*entryPoint)(), bool isKernel, uint32_t flags)
{
    //Create new thread instance
    Thread* result = new Thread();

    //Create stack for process
    result->stack = (uint8_t*)KernelHeap::allignedMalloc(THREAD_STACK_SIZE, THREAD_STACK_SIZE);
    MemoryOperations::memset(result->stack, 0, THREAD_STACK_SIZE);

    //Assign user stack, needs to be mapped into address space by elf loader if we are loading a .bin
    result->userStack = (uint8_t*)USER_STACK;
    result->userStackSize = USER_STACK_SIZE;

    //Create cpu registers for thread
    result->regsPtr = (CPUState*)((uint32_t)result->stack + THREAD_STACK_SIZE - sizeof(CPUState));

    //Set the stack pointer
    result->regsPtr->ESP = (uint32_t)result->stack + THREAD_STACK_SIZE;

    //And set the instruction pointer
    result->regsPtr->EIP = (uint32_t)entryPoint;

    //Also update the segment registers for a kernel of userspace thread
    result->regsPtr->CS = isKernel ? SEG_KERNEL_CODE : SEG_USER_CODE;
	result->regsPtr->DS = isKernel ? SEG_KERNEL_DATA : SEG_USER_DATA;
	result->regsPtr->ES = isKernel ? SEG_KERNEL_DATA : SEG_USER_DATA;
	result->regsPtr->FS = isKernel ? SEG_KERNEL_DATA : SEG_USER_DATA;
	result->regsPtr->GS = isKernel ? SEG_KERNEL_DATA : SEG_USER_DATA;

    //Set the flags for this thread
	result->regsPtr->EFLAGS = 0x202;

    //Default state is wait here
    result->state = ThreadState::Ready;

    //The thread does not have a parent by default
    result->parent = 0;

    //Return the result
    return result;
}

void ThreadHelper::RemoveThread(Thread* thread)
{
    KernelHeap::allignedFree(thread->stack);
    for(uint32_t i = (uint32_t)thread->userStack; i < (uint32_t)thread->userStack + thread->userStackSize; i+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(i, false));

    thread->state = ThreadState::Stopped;
}