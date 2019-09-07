#include <system/vfs/listingmanager.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

// Threads that have also requested a filelist before an other one was finished.
List<Thread*> waitingQueue;
// Current thread which has requested a list
Thread* currentReqThread;
// Are we currently handling a request?
bool requestBusy = false;
// List which holds the files in the current requested directory.
List<char*>* currentDirectoryList = 0;

int ListingManager::BeginListing(Thread* thread, char* path)
{
    if(!System::vfs->DirectoryExists(path))
        return 0;
    
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
int ListingManager::GetEntry(Thread* thread, int entry, char* buf)
{
    if(currentReqThread != thread)
    {
        Log(Error, "Thread requested entry while it was not the original requestor");
        return 0;
    }

    if(entry >= 0 && currentDirectoryList->size() > entry)
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
void ListingManager::EndListing(Thread* thread)
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