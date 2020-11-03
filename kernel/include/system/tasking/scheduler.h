#ifndef __CACTUSOS__SYSTEM__TASKING__SCHEDULER_H
#define __CACTUSOS__SYSTEM__TASKING__SCHEDULER_H

#include <common/types.h>
#include <system/interruptmanager.h>
#include <system/tasking/thread.h>

namespace CactusOS
{
    namespace system
    {
        #define SCHEDULER_FREQUENCY 30

        class Scheduler : public InterruptHandler
        {
        private:
            common::uint32_t frequency = 0;
            common::uint32_t tickCount = 0;

            List<Thread*> threadsList;
            Thread* currentThread = 0;

            Thread* GetNextReadyThread();
            void ProcessSleepingThreads();

            bool switchForced = false; //Is the current switch forced by a forceSwitch() call?
        public:
            bool Enabled = true;
            Scheduler();

            common::uint32_t HandleInterrupt(common::uint32_t esp);

            void AddThread(Thread* thread, bool forceSwitch = false);
            void ForceSwitch();

            Thread* CurrentThread();
            Process* CurrentProcess();

            void InitialThreadUserJump(Thread* thread);

            //Blocking and unblocking
            void Block(Thread* thread, BlockedState reason = BlockedState::Unkown);
            void Unblock(Thread* thread, bool forceSwitch = false);
        };
    }   
}

#endif