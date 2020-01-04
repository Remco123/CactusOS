#include <system/listings/processlisting.h>

#include <system/system.h>
#include <../../lib/include/shared.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

List<Process*>* processListCopy = 0;

ProcessListing::ProcessListing()
: ListingController() { }

int ProcessListing::BeginListing(Thread* thread, uint32_t unused)
{    
    if(requestBusy)
    {
        waitingQueue.push_back(thread);
        System::scheduler->Block(thread);
    }

    requestBusy = true;
    currentReqThread = thread;

    System::scheduler->Enabled = false;

    processListCopy = new List<Process*>();
    for(Process* proc : ProcessHelper::Processes)
        processListCopy->push_back(proc);

    System::scheduler->Enabled = true;

    return processListCopy->size();
}

int ProcessListing::GetEntry(Thread* thread, int entry, uint32_t bufPtr)
{
    if(currentReqThread != thread)
    {
        Log(Error, "Thread requested entry while it was not the original requestor");
        return 0;
    }

    if(entry >= 0 && processListCopy->size() > entry && bufPtr != 0)
    {
        Process* item = processListCopy->GetAt(entry);
        if(item == 0)
            return 0;

        LIBCactusOS::ProcessInfo* targetInfo = (LIBCactusOS::ProcessInfo*)bufPtr;
        
        // Copy info to target
        MemoryOperations::memcpy(targetInfo->fileName, item->fileName, sizeof(targetInfo->fileName));
        targetInfo->heapMemory = item->heap.heapEnd - item->heap.heapStart;
        targetInfo->id = item->id;
        targetInfo->isUserspace = item->isUserspace;
        targetInfo->syscallID = item->syscallID;
        targetInfo->threads = item->Threads.size();
        
        return 1;
    }
    
    // End of items
    return 0;
}

void ProcessListing::EndListing(Thread* thread)
{
    if(currentReqThread != thread || currentReqThread == 0)
    {
        Log(Error, "Thread requested EndListing while it was not the original requestor");
        return;
    }

    requestBusy = false;
    currentReqThread = 0;
    delete processListCopy;

    if(waitingQueue.size() > 0) //Unblock first thread from queue.
        System::scheduler->Unblock(waitingQueue[0]);
}