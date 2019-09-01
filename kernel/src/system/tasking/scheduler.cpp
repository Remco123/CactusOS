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
    this->currentThread = 0;
    this->threadsList.Clear();
    this->Enabled = false;
    this->switchForced = false;
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
    BootConsole::Write("CS: 0x"); Print::printfHex16(regs->CS);   BootConsole::Write("   ");   
    BootConsole::Write("DS: 0x"); Print::printfHex16(regs->DS);   BootConsole::Write("   ");
    BootConsole::Write("ES: 0x"); Print::printfHex16(regs->ES);   BootConsole::WriteLine();
    BootConsole::Write("FS: 0x"); Print::printfHex16(regs->FS);   BootConsole::Write("   ");
    BootConsole::Write("GS: 0x"); Print::printfHex16(regs->GS);   BootConsole::WriteLine();
    BootConsole::Write("EFLAGS: 0b"); Print::printbits((uint16_t)regs->EFLAGS); BootConsole::WriteLine();
}

uint32_t Scheduler::HandleInterrupt(uint32_t esp)
{
    tickCount++;
    if(this->switchForced == false)
        ProcessSleepingThreads();
    else
        this->switchForced = false; //Reset it back

    if(tickCount == frequency)
    {
        //Log(Info, "Performing Task Switch");
        
        //Reset tick count first
        tickCount = 0;

        if(threadsList.size() > 0 && this->Enabled)
        {
            //Get a new thread to switch to
            Thread* nextThread = GetNextReadyThread();

            //Remove all the threads that are stopped from the system
            while(nextThread->state == Stopped)
            {
                Log(Info, "Removing thread %x from system", (uint32_t)nextThread);
                threadsList.Remove(nextThread);
                delete nextThread;

                //Ask for a new thread
                nextThread = GetNextReadyThread();
            }
#if 0
            Log(Info, "Switching from %s %d to %s %d", currentThread->parent->fileName, currentThread->parent->Threads.IndexOf(currentThread), nextThread->parent->fileName, nextThread->parent->Threads.IndexOf(nextThread));
            BootConsole::WriteLine("-- Current Registers --");
            printRegs((CPUState*)esp);
            BootConsole::Write("cr3: 0x"); Print::printfHex32(currentThread->parent->pageDirPhys); BootConsole::WriteLine();
            BootConsole::WriteLine("-- Next Registers --");
            printRegs(nextThread->regsPtr);
            BootConsole::Write("cr3: 0x"); Print::printfHex32(nextThread->parent->pageDirPhys); BootConsole::WriteLine();
#endif         
            //Check if the current thread is stopped
            if(currentThread != 0 && currentThread->state == Stopped)
            {
                Log(Info, "Removing thread %x from system", (uint32_t)currentThread);
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

                //Save current fpu status
                asm volatile ("fxsave (%%eax)" : : "a" (currentThread->FPUBuffer));
            }

            //Load fpu status
            asm volatile ("fxrstor (%%eax)" : : "a" (nextThread->FPUBuffer));
            
            //Since we are switching now
            currentThread = nextThread;

            //Check if the next thread has not been called before
            if(nextThread->state == Started && nextThread->parent && nextThread->parent->isUserspace)
            {
                nextThread->state = ThreadState::Ready;

                Log(Info, "Jumping to new created thread");
                InitialThreadUserJump(nextThread);
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
    int currentThreadIndex = (currentThread != 0 ? threadsList.IndexOf(currentThread) : 0);
    currentThreadIndex += 1;
    if(currentThreadIndex >= threadsList.size())
        currentThreadIndex = 0;

    while(threadsList[currentThreadIndex]->state == Blocked){
        currentThreadIndex += 1;
        if(currentThreadIndex >= threadsList.size())
            currentThreadIndex = 0;
    }

    return threadsList[currentThreadIndex]; 
}

void Scheduler::AddThread(Thread* thread, bool forceSwitch)
{
    threadsList.push_back(thread);

    if(forceSwitch)
    {
        if(thread->parent && thread->parent->isUserspace)
            InitialThreadUserJump(thread);

        this->ForceSwitch();
    }
}

void Scheduler::InitialThreadUserJump(Thread* thread)
{
    InterruptDescriptorTable::DisableInterrupts();

    TSS::SetStack(0x10, (uint32_t)thread->stack + THREAD_STACK_SIZE);

    //Dont forget to load the page directory
    VirtualMemoryManager::SwitchPageDirectory(thread->parent->pageDirPhys);

    //We are becoming the current thread
    currentThread = thread;

    //We need to be enabled on the next timer interrupt
    this->Enabled = true;

    //Ack Timer interrupt
    outportb(0x20, 0x20);

    enter_usermode(thread->regsPtr->EIP, (uint32_t)thread->userStack + thread->userStackSize);
}

void Scheduler::ForceSwitch()
{
    this->switchForced = true;
    this->Enabled = true;
    this->tickCount = frequency - 1;
    asm volatile ("int $0x20"); //Call timer interrupt
}

Thread* Scheduler::CurrentThread()
{
    return currentThread; 
}
Process* Scheduler::CurrentProcess()
{
    return currentThread->parent;
}

void Scheduler::Block(Thread* thread, BlockedState reason)
{
    //Log(Info, "Blocking thread %x", (uint32_t)thread);
    thread->blockedState = reason;
    thread->state = ThreadState::Blocked;

    if(thread == CurrentThread())
        ForceSwitch();
}
void Scheduler::Unblock(Thread* thread, bool forceSwitch)
{
    //Log(Info, "Unblocking thread %x", (uint32_t)thread);
    thread->state = ThreadState::Ready;

    if(forceSwitch)
        ForceSwitch();
}
void Scheduler::ProcessSleepingThreads()
{
    for(int i = 0; i < threadsList.size(); i++)
    {
        Thread* thread = threadsList[i];
        if(thread->state == Blocked && thread->blockedState == SleepMS && thread->timeDelta > 0)
        {
            thread->timeDelta--;
            if(thread->timeDelta <= 0)
                Unblock(thread); //Perhaps call thread directly when time has passed, this will be more acurate but harder to implement
        }
    }
}