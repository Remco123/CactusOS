#include <system/tasking/scheduler.h>

#include <system/system.h>
#include <core/tss.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

extern "C" void enter_usermode(uint32_t location, uint32_t stackAddress, uint32_t flags);

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

    enter_usermode(thread->regsPtr->EIP, (uint32_t)thread->userStack + thread->userStackSize, thread->regsPtr->EFLAGS);
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
    if(currentThread)
        return currentThread->parent;
    return 0;
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