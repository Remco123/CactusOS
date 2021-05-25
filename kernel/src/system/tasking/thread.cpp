#include <system/tasking/thread.h>

#include <system/memory/heap.h> 
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

ThreadHelper::ThreadHelper()
{   }

Thread* ThreadHelper::CreateFromFunction(void (*entryPoint)(), bool isKernel, uint32_t flags, Process* parent)
{
    //Create new thread instance
    Thread* result = new Thread();

    //Create stack for process
    result->stack = (uint8_t*)KernelHeap::alignedMalloc(THREAD_STACK_SIZE, THREAD_STACK_SIZE);
    MemoryOperations::memset(result->stack, 0, THREAD_STACK_SIZE);

    //Assign user stack, needs to be mapped into address space by elf loader if we are loading a .bin
    //When a process requests a new thread the stack for the new thread needs to be moved down, otherwise the stacks will be equal.
    //The memory required is mapped by the elfloader or by the syscall handler.
    result->userStack = (parent == 0 ? (uint8_t*)USER_STACK : (uint8_t*)(USER_STACK - USER_STACK_SIZE*parent->Threads.size()));
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
	result->regsPtr->EFLAGS = flags;

    //Default state is wait here
    result->state = ThreadState::Ready;

    //Assign thread parent
    result->parent = parent;

    //Create a buffer for the fpu
    result->FPUBuffer = (uint8_t*)KernelHeap::alignedMalloc(512, 16);
    MemoryOperations::memset(result->FPUBuffer, 0, 512);

    //Return the result
    return result;
}

void ThreadHelper::RemoveThread(Thread* thread)
{
    KernelHeap::allignedFree(thread->stack);
    KernelHeap::allignedFree(thread->FPUBuffer);
    for(uint32_t i = (uint32_t)thread->userStack; i < (uint32_t)thread->userStack + thread->userStackSize; i+=PAGE_SIZE)
        VirtualMemoryManager::FreePage(VirtualMemoryManager::GetPageForAddress(i, false));

    thread->state = ThreadState::Stopped;
}