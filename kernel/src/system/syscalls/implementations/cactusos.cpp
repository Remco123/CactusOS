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

    switch (sysCall)
    {
        case SYSCALL_EXIT:
            Log(Info, "Process %d (%s) exited with code %d", proc->id, proc->fileName, (int)state->EBX);
            ProcessHelper::RemoveProcess(proc);
            state->EAX = SYSCALL_RET_SUCCES;
            break;
        case SYSCALL_LOG:
            Log((LogLevel)state->EBX, (const char* __restrict__)state->ECX);
            state->EAX = SYSCALL_RET_SUCCES;
            break;
        case SYSCALL_GUI_GETLFB:
            VirtualMemoryManager::mapVirtualToPhysical((void*)System::vesa->currentVideoMode.PhysBasePtr, (void*)state->EBX, System::vesa->GetBufferSize(), false, true);
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
        default:
            Log(Warning, "Got not supported syscall %d from process %d", sysCall, proc->id);
            state->EAX = SYSCALL_RET_ERROR;
            break;
    }

    return state;
}