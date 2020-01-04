#include <system/listings/disklisting.h>
#include <system/system.h>
#include <../../lib/include/shared.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

DiskListing::DiskListing()
: ListingController() { }

int DiskListing::BeginListing(Thread* thread, uint32_t unused)
{    
    if(requestBusy)
    {
        waitingQueue.push_back(thread);
        System::scheduler->Block(thread);
    }

    requestBusy = true;
    currentReqThread = thread;

    return System::diskManager->allDisks.size();
}
int DiskListing::GetEntry(Thread* thread, int entry, uint32_t bufPtr)
{
    if(currentReqThread != thread)
    {
        Log(Error, "Thread requested entry while it was not the original requestor");
        return 0;
    }

    if(entry >= 0 && System::diskManager->allDisks.size() > entry && bufPtr != 0)
    {
        LIBCactusOS::DiskInfo* targ = (LIBCactusOS::DiskInfo*)bufPtr;
        Disk* src = System::diskManager->allDisks[entry];
        MemoryOperations::memcpy(targ->identifier, src->identifier, String::strlen(src->identifier));
        targ->identifier[String::strlen(src->identifier)] = '\0';

        return sizeof(LIBCactusOS::DiskInfo);
    }
    
    // End of items
    return 0;
}
void DiskListing::EndListing(Thread* thread)
{
    if(currentReqThread != thread || currentReqThread == 0)
    {
        Log(Error, "Thread requested listing end while it was not the original requestor");
        return;
    }

    requestBusy = false;
    currentReqThread = 0;

    if(waitingQueue.size() > 0) //Unblock first thread from queue.
        System::scheduler->Unblock(waitingQueue[0]);
}