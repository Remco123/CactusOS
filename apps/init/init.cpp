#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include <ipc.h>
#include <time.h>
#include <math.h>
#include <gui/gui.h>
#include <imaging/image.h>
#include "progress.h"

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

char* path = "B:\\boot.jpg";

int main(int argc, char** argv)
{
    Log(Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() != SYSCALL_RET_SUCCES)
        return -1;

    DirectGUI::Clear(0xFFFFFFFF);

    int x_p = GUI::Width/2 - 100;
    int y_p = GUI::Width/2 - 200;

    ProgressBar* bar = new ProgressBar(x_p, y_p + 250, 200, 10);
    bar->SetValue(0);

    Log(Info, "Loading Boot Logo");
    Image* logo = Image::CreateFromFile(path);
    if(logo)
        logo->DrawTo(DirectGUI::GetCanvas(), GUI::Width / 2 - logo->GetWidth()/2, GUI::Height / 2 - logo->GetHeight()/2);

    bar->SetValue(70);

    Print("Launched Compositor pid: %d\n", Process::Run("B:\\apps\\compositor.bin"));

    // Prevent that one of the other launch processes gets the PID of 3
    // There can be (a very small) chance that the desktop will get launched before the compositor if there is a weird task switch
    while (Process::Active(GUI::compositorPID) == false)
        Time::Sleep(100);
    
    Print("Launched Desktop pid: %d\n", Process::Run("B:\\apps\\desktop.bin"));
    Print("Launched Clock pid: %d\n", Process::Run("B:\\apps\\clock.bin"));
    //Print("Launched Sysinfo pid: %d\n", Process::Run("B:\\apps\\powermanager.bin"));

    bar->SetValue(100); 

    return 0;
}