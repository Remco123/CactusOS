#ifndef __CACTUSOS__SYSTEM__TASKING__SCHEDULER_H
#define __CACTUSOS__SYSTEM__TASKING__SCHEDULER_H

#include <common/types.h>
#include <common/list.h>
#include <core/idt.h>
#include <core/registers.h>
#include <core/tss.h>
#include <system/interruptmanager.h>
#include <system/tasking/procces.h>

namespace CactusOS
{
    namespace system
    {
        #define DECLARE_LOCK(name) volatile int name ## Locked
        #define LOCK(name) \
	        while (!__sync_bool_compare_and_swap(& name ## Locked, 0, 1)); \
	        __sync_synchronize();
        #define UNLOCK(name) \
	        __sync_synchronize(); \
	        name ## Locked = 0;

        #define SCHEDULER_FREQ 30
        class Scheduler : public InterruptHandler
        {
        private:
            common::uint32_t frequency = 0;
            common::uint32_t tickCount = 0;

            common::uint32_t currentThread = 0;
        public:
            List<Thread*> threadList;
            
            Scheduler(common::uint32_t freq);

            //Called every timer tick
            common::uint32_t HandleInterrupt(common::uint32_t esp);

            Thread* GetCurrentThread();
            Procces* GetCurrentProcces();
            void ForceSwitch();
        };
    }
}

#endif