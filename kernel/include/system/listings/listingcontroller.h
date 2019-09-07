#ifndef __CACTUSOS__SYSTEM__VFS__LISTINGCONTROLLER_H
#define __CACTUSOS__SYSTEM__VFS__LISTINGCONTROLLER_H

#include <system/vfs/virtualfilesystem.h>
#include <system/tasking/process.h>

namespace CactusOS
{
    namespace system
    {
        // A class that creates an easy interface for threads to request a list from the kernel.
        // This can for example be a list of files or processes.
        class ListingController
        {
        protected:
            // Threads that have also requested a listing before an other one was finished.
            List<Thread*> waitingQueue;
            
            // Current thread which has requested a list
            Thread* currentReqThread;

            // Are we currently handling a request?
            bool requestBusy = false;
        public:
            ListingController();

            virtual int BeginListing(Thread* thread, uint32_t arg1 = 0);
            virtual int GetEntry(Thread* thread, int entry, uint32_t bufPtr);
            virtual void EndListing(Thread* thread);
        };
    }
}

#endif