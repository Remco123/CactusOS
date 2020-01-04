#ifndef __CACTUSOS__SYSTEM__LISTINGS__DISKLISTING_H
#define __CACTUSOS__SYSTEM__LISTINGS__DISKLISTING_H

#include <system/listings/listingcontroller.h>

namespace CactusOS
{
    namespace system
    {
        // A class that is responsable for processes which request the list of disks on the system
        class DiskListing : public ListingController
        {
        public:
            /**
            Create new instance of DiskListing
            */
            DiskListing();

            /**
            Begin processing a new disklist request
            Returns amount of disks present on system
            */
            int BeginListing(Thread* thread, uint32_t unused) override;
            /**
            Get an item from the current request.
            Returns characters in disk-name/id
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