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

bool shouldExit;

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

void TestThread()
{
    int i = 0;
    while (1) {
        Print("Test Loop\n");
        Time::Sleep(100);
        i++;
        if(i == 12) {
            shouldExit = true;
        }
    }
}

int threads = 0;

void TestThread2()
{
    while(1) {
        Print("Eat\n");
        Time::Sleep(100);
        if(threads++ < 5)
            Process::CreateThread(TestThread2, false);
    }
}

int main()
{
    shouldExit = false;
    threads = 0;
    Print("Test application started! PID=%d\n", Process::ID);

    if(DirectGUI::RequestFramebuffer())
    {
        for(int x = 0; x < 5; x++)
            for(int y = 0; y < 4; y++) {
                Draw3DCube(200 * x + 50, 160 * y + 50, 0xFF00CC44 * (x+1) * (y+1));
            }
    }

    Print("Requesting new thread should_exit=%d\n", shouldExit);
    Process::CreateThread(TestThread, false);
    Process::CreateThread(TestThread2, true);

    while(!shouldExit) {
        Print("Yield\n");
        Time::Sleep(1000);
    }

    Print("Exit\n");
    DoSyscall(SYSCALL_EXIT);

    return 0;
}