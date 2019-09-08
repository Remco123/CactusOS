#ifndef __CACTUSOS__SYSTEM__LISTINGS__DIRECTORYLISTING_H
#define __CACTUSOS__SYSTEM__LISTINGS__DIRECTORYLISTING_H

#include <system/listings/listingcontroller.h>

namespace CactusOS
{
    namespace system
    {
        // A class that is responsable for processes which request a directory list
        class DirectoryListing : public ListingController
        {
        public:
            /**
            Create new instance of DirectoryListing
            */
            DirectoryListing();

            /**
            Begin processing a new directorylist request
            Returns amount of items in directory
            */
            int BeginListing(Thread* thread, uint32_t pathPtr) override;
            /**
            Get an item from the current request.
            Returns characters in file/dirname
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