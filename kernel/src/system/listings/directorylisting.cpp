#include <system/listings/directorylisting.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

// List which holds the files in the current requested directory.
List<char*>* currentDirectoryList = 0;

DirectoryListing::DirectoryListing()
: ListingController() { }

int DirectoryListing::BeginListing(Thread* thread, uint32_t pathPtr)
{
    char* path = (char*)pathPtr;
    if(!System::vfs->DirectoryExists(path))
        return -1;
    
    if(requestBusy)
    {
        waitingQueue.push_back(thread);
        System::scheduler->Block(thread);
    }

    requestBusy = true;
    currentReqThread = thread;

    currentDirectoryList = System::vfs->DirectoryList(path);
    return currentDirectoryList->size();
}
int DirectoryListing::GetEntry(Thread* thread, int entry, uint32_t bufPtr)
{
    char* buf = (char*)bufPtr;
    if(currentReqThread != thread)
    {
        Log(Error, "Thread requested entry while it was not the original requestor");
        return 0;
    }

    if(entry >= 0 && currentDirectoryList->size() > entry && buf != 0)
    {
        char* str = currentDirectoryList->GetAt(entry);
        int len = String::strlen(str);

        if(len > (100-1))
            return 0; //String is to large

        MemoryOperations::memcpy(buf, str, len);
        buf[len] = '\0';

        delete str; //Free memory used by string

        return len;
    }
    
    // End of items
    return 0;
}
void DirectoryListing::EndListing(Thread* thread)
{
    if(currentReqThread != thread || currentReqThread == 0)
    {
        Log(Error, "Thread requested listing end while it was not the original requestor");
        return;
    }

    requestBusy = false;
    currentReqThread = 0;
    delete currentDirectoryList;

    if(waitingQueue.size() > 0) //Unblock first thread from queue.
        System::scheduler->Unblock(waitingQueue[0]);
}