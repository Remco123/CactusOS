#include <system/syscalls/implementations/cactusos.h>

#include <../../lib/include/syscall.h>
#include <../../lib/include/datetime.h>

#include <system/system.h>
#include <system/tasking/ipcmanager.h>
#include <core/power.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

CPUState* CactusOSSyscalls::HandleSyscall(CPUState* state)
{
    uint32_t sysCall = state->EAX;
    Process* proc = System::scheduler->CurrentProcess();
    //System::scheduler->Enabled = false;

    switch (sysCall)
    {
        case SYSCALL_EXIT:
            Log(Info, "Process %d %s exited with code %d", proc->id, proc->fileName, (int)state->EBX);
            ProcessHelper::RemoveProcess(proc);
            state->EAX = SYSCALL_RET_SUCCES;
            break;
        case SYSCALL_LOG:
            Log((LogLevel)state->EBX, (const char* __restrict__)state->ECX);
            state->EAX = SYSCALL_RET_SUCCES;
            break;
        case SYSCALL_GUI_GETLFB:
            VirtualMemoryManager::mapVirtualToPhysical((void*)System::gfxDevice->framebufferPhys, (void*)state->EBX, System::gfxDevice->GetBufferSize(), false, true);
            state->EAX = SYSCALL_RET_SUCCES;
            Log(Info, "Mapped LFB for process %d to virtual address %x", proc->id, state->EBX);
            break;
        case SYSCALL_FILE_EXISTS:
            state->EAX = System::vfs->FileExists((char*)state->EBX);
            break;
        case SYSCALL_DIR_EXISTS:
            state->EAX = System::vfs->DirectoryExists((char*)state->EBX);
            break;
        case SYSCALL_GET_FILESIZE:
            state->EAX = System::vfs->GetFileSize((char*)state->EBX);
            break;
        case SYSCALL_READ_FILE:
            state->EAX = System::vfs->ReadFile((char*)state->EBX, (uint8_t*)state->ECX);
            break;
        case SYSCALL_GET_HEAP_START:
            state->EAX = proc->heap.heapStart;
            break;
        case SYSCALL_GET_HEAP_END:
            state->EAX = proc->heap.heapEnd;
            break;
        case SYSCALL_PRINT:
            Print((const char*)state->EBX, state->ECX);
            break;
        case SYSCALL_SET_HEAP_SIZE:
            ProcessHelper::UpdateHeap(proc, state->EBX);
            state->EAX = SYSCALL_RET_SUCCES;
            break;
        case SYSCALL_RUN_PROC:
            {
                char* applicationPath = (char*)state->EBX;
                if(System::vfs->FileExists(applicationPath) == false) {
                    state->EAX = SYSCALL_RET_ERROR;
                    break;
                }

                Process* newProc = ProcessHelper::Create(applicationPath, false);
                newProc->Threads[0]->state = Started;
                if(newProc != 0) {
                    System::scheduler->AddThread(newProc->Threads[0], false);
                    state->EAX = SYSCALL_RET_SUCCES;
                }
                else
                    state->EAX = SYSCALL_RET_ERROR;
            }
            break;
        case SYSCALL_SLEEP_MS:
            {
                Thread* currentThread = System::scheduler->CurrentThread();
                currentThread->timeDelta = state->EBX;
                System::scheduler->Block(currentThread, BlockedState::SleepMS);
            }
            break;
        case SYSCALL_CREATE_SHARED_MEM:
            {
                Process* proc2 = ProcessHelper::ProcessById(state->EBX);
                state->EAX = SharedMemory::CreateSharedRegion(proc, proc2, state->ECX, state->EDX, state->ESI);
            }
            break;
        case SYSCALL_IPC_SEND:
            {
                IPCManager::HandleSend(state, proc);
            }
            break;
        case SYSCALL_IPC_RECEIVE:
            {
                IPCManager::HandleReceive(state, proc);
            }
            break;
        case SYSCALL_IPC_AVAILABLE:
            {
                state->EAX = proc->ipcMessages.size();
            }
            break;
        case SYSCALL_START_THREAD:
            {
                Log(Info, "Creating new thread for proc %d %s, jumps to %x", proc->id, proc->fileName, state->EBX);
                //Create new thread
                Thread* newThread = ThreadHelper::CreateFromFunction((void (*)())state->EBX, false, 514, proc);
                
                //Create memory for stack
                for(uint32_t i = (uint32_t)newThread->userStack; i < ((uint32_t)newThread->userStack + newThread->userStackSize); i+=PAGE_SIZE)
                    VirtualMemoryManager::AllocatePage(VirtualMemoryManager::GetPageForAddress(i, true, true, true), false, true);

                //Assign parent
                newThread->parent = proc;
                //Set it as started
                newThread->state = Started;
                //Add it to the parent process
                proc->Threads.push_back(newThread);
                //Add it to the scheduler
                System::scheduler->AddThread(newThread);
                //Force a task switch if the application requested it
                if((bool)state->ECX)
                    System::scheduler->ForceSwitch();
            }
            break;
        case SYSCALL_MAP_SYSINFO:
            {
                //Put systeminfo into address space
                uint32_t sysInfoPhys = (uint32_t)VirtualMemoryManager::virtualToPhysical((void*)System::systemInfo);
                PageTableEntry* sysInfoPage = VirtualMemoryManager::GetPageForAddress(state->EBX, true, true, proc->isUserspace);
                sysInfoPage->readWrite = 1;
                sysInfoPage->isUser = proc->isUserspace;
                sysInfoPage->frame = sysInfoPhys / PAGE_SIZE;
                sysInfoPage->present = 1;

                state->EAX = SYSCALL_RET_SUCCES;
            }
            break;
        case SYSCALL_YIELD:
            {
                System::scheduler->ForceSwitch();
                state->EAX = SYSCALL_RET_SUCCES;
            }
            break;
        case SYSCALL_GET_TICKS:
            {
                uint64_t* resultPtr = (uint64_t*)state->EBX;
                *resultPtr = System::pit->Ticks();
                state->EAX = SYSCALL_RET_SUCCES;
            }
            break;
        case SYSCALL_GET_DATETIME:
            {
                LIBCactusOS::DateTime* resultPtr = (LIBCactusOS::DateTime*)state->EBX;
                resultPtr->Day = System::rtc->GetDay();
                resultPtr->Hours = System::rtc->GetHour();
                resultPtr->Minutes = System::rtc->GetMinute();
                resultPtr->Month = System::rtc->GetMonth();
                resultPtr->Seconds = System::rtc->GetSecond();
                resultPtr->Year = System::rtc->GetYear();
                state->EAX = SYSCALL_RET_SUCCES;
            }
            break;
        case SYSCALL_SHUTDOWN:
            {
                Log(Info, "Process requested shutdown");
                Power::Poweroff();
                
                //We should not return after shutting down
                state->EAX = SYSCALL_RET_ERROR;
            }
            break;
        case SYSCALL_REBOOT:
            {
                Log(Info, "Process requested reboot");
                Power::Reboot();

                //We should not return from a reboot :)
                state->EAX = SYSCALL_RET_ERROR;
            }
            break;
        case SYSCALL_EJECT_DISK:
            {
                state->EAX = System::vfs->EjectDrive((char*)state->EBX);
            }
            break;
        case SYSCALL_READ_STDIO:
            {
                if(proc->stdInput != 0)
                {
                    while (proc->stdInput->Availible() <= 0) //TODO: Use blocking here
                        System::scheduler->ForceSwitch();
                    state->EAX = proc->stdInput->Read();
                }
                else
                    Log(Warning, "StdIn is zero for process %s", proc->fileName);
            }
            break;
        case SYSCALL_WRITE_STDIO:
            {
                if(proc->stdOutput != 0)
                    proc->stdOutput->Write((char)state->EBX);
                else
                    Log(Warning, "StdOut is zero for process %s", proc->fileName);
            }
            break;
        case SYSCALL_REDIRECT_STDIO:
            {

            }
            break;
        default:
            Log(Warning, "Got unkown syscall %d from process %d", sysCall, proc->id);
            state->EAX = SYSCALL_RET_ERROR;
            break;
    }

    //System::scheduler->Enabled = true;

    return state;
}