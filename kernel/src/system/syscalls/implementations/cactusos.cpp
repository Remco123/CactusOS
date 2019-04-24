#include <system/syscalls/implementations/cactusos.h>

#include <../../lib/include/syscall.h>

#include <system/system.h>

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
            Log(Info, "Process %d exited with code %d", proc->id, (int)state->EBX);
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
                Process* proc = ProcessHelper::Create(applicationPath, false);
                if(proc != 0) {
                    System::scheduler->AddThread(proc->Threads[0], true);
                    state->EAX = SYSCALL_RET_SUCCES;
                }
                else
                    state->EAX = SYSCALL_RET_ERROR;
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