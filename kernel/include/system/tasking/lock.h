#ifndef __CACTUSOS__SYSTEM__TASKING__LOCK_H
#define __CACTUSOS__SYSTEM__TASKING__LOCK_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class MutexLock
        {
        private:
            int value = 0;
        public:
            MutexLock();

            void Lock();
            void Unlock();
        };
    }
}

#endif