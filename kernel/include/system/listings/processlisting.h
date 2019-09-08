#ifndef __CACTUSOS__SYSTEM__LISTINGS__PROCESSLISTING_H
#define __CACTUSOS__SYSTEM__LISTINGS__PROCESSLISTING_H

#include <system/listings/listingcontroller.h>

namespace CactusOS
{
    namespace system
    {
        // A class that is responsable for a request of a list of processes
        class ProcessListing : public ListingController
        {
        public:
            /**
            Create new instance of ProcessListing
            */
            ProcessListing();

            /**
            Begin processing a new request
            Returns amount of processes
            */
            int BeginListing(Thread* thread, uint32_t unused) override;
            /**
            Get an item from the current request.
            Returns 1 for succes and 0 for failure
            */
            int GetEntry(Thread* thread, int entry, uint32_t bufPtr) override;
            /**
            End the current listing
            */
            void EndListing(Thread* thread) override;
        };
    }
}

#endif