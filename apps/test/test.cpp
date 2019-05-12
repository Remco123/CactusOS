#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <time.h>
#include <proc.h>
#include <ipc.h>

using namespace LIBCactusOS;

void Draw3DCube(int x, int y, uint32_t color)
{
    const int w = 40;
    const int b = 0xFF000000;
    for(int i = 0; i <= w; i++)
        DirectGUI::DrawFillRect(color, x + i, y + i, 100, 100);

    DirectGUI::DrawLine(b, x, y, x + 100, y);
    DirectGUI::DrawLine(b, x, y, x + w, y + w);
    DirectGUI::DrawLine(b, x + 100 - 1, y, x + w + 100 - 1, y + w);
    DirectGUI::DrawLine(b, x + w, y + w, x + w + 100, y + w);
    DirectGUI::DrawLine(b, x, y, x, y + 100);
    DirectGUI::DrawLine(b, x + w, y + w, x + w, y + 100 + w);
    DirectGUI::DrawLine(b, x + 100 + w - 1, y + w, x + 100 + w - 1, y + 100 + w);
    DirectGUI::DrawLine(b, x, y + 100, x + w, y + 100 + w);
    DirectGUI::DrawLine(b, x + w, y + 100 + w, x + w + 100,y + 100 + w);
}

int main()
{
    Print("Test application started! PID=%d\n", Process::ID);

    IPCMessage message = ICPReceive();
    Print("Message: source=%d dest=%d type=%d args=%d,%d,%d,%d,%d\n", message.source, message.dest, message.type, message.arg1, message.arg2, message.arg3, message.arg4, message.arg5);

    if(DirectGUI::RequestFramebuffer())
    {
        for(int x = 0; x < 5; x++)
            for(int y = 0; y < 4; y++) {
                Draw3DCube(200 * x + 50, 160 * y + 50, 0xFF00CC44 * (x+1) * (y+1));
                Time::Sleep(500);
            }
    }    

    while(true)
    {
        DirectGUI::Clear(0xFFFFFFFF);
        Time::Sleep(1000);
        DirectGUI::Clear(0xFF333333);
        Time::Sleep(1000);
    }

    return 0;
}