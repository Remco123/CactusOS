#include <system/tasking/process.h>

#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

static int currentPID = 1;

ProcessHelper::ProcessHelper()
{   }

Process* ProcessHelper::CreateFromBin(char* fileName, bool isKernel)
{
    //Check if the file exists
    if(!System::vfs->FileExists(fileName))
        return 0;

    //Get the filesize
    int fileSize = System::vfs->GetFileSize(fileName);
    if(fileSize == -1)
        return 0;

    //Allocate a buffer to read the bin
    uint8_t* fileBuffer = new uint8_t[fileSize];

    if(System::vfs->ReadFile(fileName, fileBuffer) != 0) //An error occured
    {
        delete fileBuffer;
        return 0;
    }

    //Create new process info block
    Process* result = new Process();

    //And set it to zero
    MemoryOperations::memset(result, 0, sizeof(Process));

    //Create main thread for process
    Thread* mainThread = ThreadHelper::CreateFromFunction((void (*)())fileBuffer, isKernel);

    //Set the thread parent to the process we are creating
    mainThread->parent = result;
    
    //Setup process
    MemoryOperations::memcpy(result->fileName, fileName, String::strlen(fileName));

    //Set the pid
    result->id = currentPID++;

    //Set the process as active
    result->state = ProcessState::Active;

    //And assign the main thread
    result->Threads.push_back(mainThread);

    //Finally return result
    return result;
}   