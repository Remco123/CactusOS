#include <gui/directgui.h>
#include <log.h>
#include <syscall.h>

using namespace LIBCactusOS;

bool DirectGUI::RequestFramebuffer()
{
    Log(Info, "This process is requesting a direct framebuffer");

    return DoSyscall(SYSCALL_GUI_GETLFB, DIRECT_GUI_ADDR);
}