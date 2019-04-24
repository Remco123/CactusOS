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
    Print("Test application started!\n");

    if(DirectGUI::RequestFramebuffer())
    {
        DirectGUI::DrawCircle(0xFFFF0000, 100, 100, 50);
        DirectGUI::DrawFillRect(0xFF000000, 700, 0, 300, 300);
    }    

    for(int i = 0; i < 30000; i++)
    {
        DirectGUI::DrawCircle(0xFFFF0000 ^ i, 100, 100, 50);
    }

    Print("Drawing is done!\n");

    return 0;
}