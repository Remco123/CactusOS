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
    this->Threads.Clear();
    this->frequency = freq;
    this->tickCount = 0;
}

//Called every timer tick
uint32_t Scheduler::HandleInterrupt(uint32_t esp)
{
    tickCount++;

    if(tickCount == frequency)
    {
        //Peform task switch
        tickCount = 0;

        if(Threads.size() <= 0)
            return esp;

        if(esp > KERNEL_HEAP_START)
            Threads[currentThread]->cpuState = (CPUState*)esp;

        if(++currentThread >= Threads.size())
            currentThread %= Threads.size();

        esp = (uint32_t)Threads[currentThread]->cpuState;

        //Update tss
        //TSS::SetStack(Threads[currentThread]->cpuState->UserSS, Threads[currentThread]->cpuState->UserESP);
    }

    return esp;
}