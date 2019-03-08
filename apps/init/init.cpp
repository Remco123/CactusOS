#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>

using namespace LIBCactusOS;

int main()
{
    API::Initialize();

    Log(Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() == SYSCALL_RET_SUCCES)
    {
        DirectGUI::Clear(0xFFFFFFFF);
    }

    return 0;
}