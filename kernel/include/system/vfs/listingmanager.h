#ifndef __CACTUSOS__SYSTEM__VFS__LISTINGMANAGER_H
#define __CACTUSOS__SYSTEM__VFS__LISTINGMANAGER_H

#include <system/vfs/virtualfilesystem.h>
#include <system/tasking/process.h>

namespace CactusOS
{
    namespace system
    {
        // A class that is responsable for processes which request a list of files
        class ListingManager
        {
        public:
            /**
            Begin processing a new filelist request
            Returns amount of items in directory
            */
            static int BeginListing(Thread* thread, char* path);
            /**
            Get an item from the current request.
            Returns characters in file/dirname
            */
            static int GetEntry(Thread* thread, int entry, char* buf);
            /**
            End the current listing
            */
            static void EndListing(Thread* thread);
        };
    }
}

#endif