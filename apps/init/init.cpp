#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>

using namespace LIBCactusOS;

int main()
{
    Log(Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() == SYSCALL_RET_SUCCES)
    {
        DirectGUI::Clear(0xFFFFFFFF);

        uint8_t* ptr = (uint8_t*)UserHeap::allignedMalloc(512, 0x1000);

        Print("Allocated memory at: %x\n", (uint32_t)ptr);
    }

    return 0;
}