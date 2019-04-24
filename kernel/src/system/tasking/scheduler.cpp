#include <system/tasking/scheduler.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

extern "C" void enter_usermode(uint32_t location, uint32_t stackAddress);

Scheduler::Scheduler()
: InterruptHandler(0x20)
{
    this->tickCount = 0;
    this->frequency = SCHEDULER_FREQUENCY;
    this->currentThreadIndex = 0;
    this->threadsList.Clear();
    this->Enabled = false;
}

void printRegs(CPUState* regs)
{
    BootConsole::Write("eax: 0x"); Print::printfHex32(regs->EAX); BootConsole::Write("   ");
    BootConsole::Write("ebx: 0x"); Print::printfHex32(regs->EBX); BootConsole::Write("   ");
    BootConsole::Write("ecx: 0x"); Print::printfHex32(regs->ECX); BootConsole::WriteLine();
    BootConsole::Write("edx: 0x"); Print::printfHex32(regs->EDX); BootConsole::Write("   ");
    BootConsole::Write("esi: 0x"); Print::printfHex32(regs->ESI); BootConsole::Write("   ");
    BootConsole::Write("edi: 0x"); Print::printfHex32(regs->EDI); BootConsole::WriteLine();
    BootConsole::Write("ebp: 0x"); Print::printfHex32(regs->EBP); BootConsole::Write("   ");
    BootConsole::Write("esp: 0x"); Print::printfHex32(regs->ESP); BootConsole::Write("   ");
    BootConsole::Write("eip: 0x"); Print::printfHex32(regs->EIP); BootConsole::WriteLine();
}

uint32_t Scheduler::HandleInterrupt(uint32_t esp)
{
    tickCount++;

    if(tickCount == frequency)
    {
        //Log(Info, "Performing Task Switch");
        
        //Reset tick count first
        tickCount = 0;

        if(threadsList.size() > 0 && this->Enabled)
        {
            Thread* currentThread = threadsList[currentThreadIndex];
            Thread* nextThread = GetNextReadyThread();

#if 1
            Log(Info, "Switching from %x %d to %x %d", (uint32_t)currentThread, currentThread->parent->isUserspace, (uint32_t)nextThread, nextThread->parent->isUserspace);
            BootConsole::WriteLine("-- Current Registers --");
            printRegs((CPUState*)esp);
            BootConsole::Write("cr3: 0x"); Print::printfHex32(currentThread->parent->pageDirPhys); BootConsole::WriteLine();
            BootConsole::WriteLine("-- Next Registers --");
            printRegs(nextThread->regsPtr);
            BootConsole::Write("cr3: 0x"); Print::printfHex32(nextThread->parent->pageDirPhys); BootConsole::WriteLine();
#endif       
            
            if(currentThread->state == Stopped)
            {
                threadsList.Remove(currentThread);
                delete currentThread;
            }

            //At the first context switch the esp is pointing at the stack pointer used by the kernel,
            //we do not want to save this info otherwise we will be running the kernel instead of the task
            //Since all the stack pointers are allocated by the kernel we can check if it is kernel or not
            //TODO: Is there no better way for this?
            else if(esp >= KERNEL_HEAP_START)
            {
                //Save old registers
                currentThread->regsPtr = (CPUState*)esp;
            }

            //Load new registers
            esp = (uint32_t)nextThread->regsPtr;

            //Load page directory
            if(nextThread->parent && nextThread->parent->pageDirPhys != 0)
                VirtualMemoryManager::SwitchPageDirectory(nextThread->parent->pageDirPhys);

            TSS::SetStack(0x10, (uint32_t)nextThread->stack + THREAD_STACK_SIZE);
        }
    }

    return esp;
}


Thread* Scheduler::GetNextReadyThread()
{
    currentThreadIndex++;
    
    if(currentThreadIndex >= threadsList.size())
        currentThreadIndex = 0;

    return threadsList[currentThreadIndex]; 
}

void Scheduler::AddThread(Thread* thread, bool forceSwitch)
{
    threadsList.push_back(thread);

    if(forceSwitch)
    {
        if(thread->parent) {
            if(thread->parent->isUserspace)
            {
                InterruptDescriptorTable::DisableInterrupts();

                TSS::SetStack(0x10, (uint32_t)thread->stack + THREAD_STACK_SIZE);

                //Dont forget to load the page directory
                VirtualMemoryManager::SwitchPageDirectory(thread->parent->pageDirPhys);

                currentThreadIndex = threadsList.size() - 1;

                this->Enabled = true;

                enter_usermode(thread->regsPtr->EIP, (uint32_t)thread->userStack + thread->userStackSize);
            }
        }
        this->ForceSwitch();
    }
}

void Scheduler::ForceSwitch()
{
    this->Enabled = true;
    this->tickCount = frequency - 1;
    asm volatile ("int $0x20"); //Call timer interrupt
}

Thread* Scheduler::CurrentThread()
{
    return threadsList[currentThreadIndex]; 
}
Process* Scheduler::CurrentProcess()
{
    return threadsList[currentThreadIndex]->parent;
}