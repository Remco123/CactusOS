#include <system/tasking/scheduler.h>

#include <system/memory/heap.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

extern "C" uintptr_t cpuGetEIP();

Scheduler::Scheduler(uint32_t freq)
: InterruptHandler(IDT_INTERRUPT_OFFSET + 0)
{
    this->threadList.Clear();
    this->frequency = freq;
    this->tickCount = 0;
}

//Called every timer tick
uint32_t Scheduler::HandleInterrupt(uint32_t esp)
{
    tickCount++;

    if(tickCount == frequency) //Peform task switch
    {
        //Reset Tick count
        tickCount = 0;

        if(threadList.size() <= 0) //We are not running any threads
            return esp;

        //Disable interrupts
        InterruptDescriptorTable::DisableInterrupts();

        if(esp > KERNEL_HEAP_START)
        {
            //Save current cpuState to thread
            MemoryOperations::memcpy(&threadList[currentThread]->cpuRegisters, (CPUState*)esp, sizeof(CPUState));
        }
        //Perform Round-Robin
        currentThread++;
        
        if(currentThread >= threadList.size())
            currentThread = 0; //Start over
        
        //Load the cpuState from the new procces
        MemoryOperations::memcpy((CPUState*)esp, &threadList[currentThread]->cpuRegisters, sizeof(CPUState));
        
        //Finally re-enable interrupts
        InterruptDescriptorTable::EnableInterrupts();
    }

    return esp;
}

Thread* Scheduler::GetCurrentThread()
{
    return threadList[currentThread];
}
Procces* Scheduler::GetCurrentProcces()
{
    threadList[currentThread]->parent;
}
void Scheduler::ForceSwitch()
{
    this->tickCount = frequency - 1;
    asm("int $0x20");
}