#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/button.h>
#include <gui/directgui.h>
#include <proc.h>
#include <syscall.h>
#include <vfs.h>
#include <log.h>

void PoweroffButtonCallback(void* sender, IEventArgs arg);
void RebootButtonCallback(void* sender, IEventArgs arg);
void PoweroffAndEjectButtonCallback(void* sender, IEventArgs arg);

int main()
{
    GUI::Initialize();

    Window* mainWindow = new Window(150, 100, WIDTH/2 - 75, HEIGHT/2 - 50);
    mainWindow->titleString = "Power Options";

    Button* shutdownButton = new Button("Poweroff");
    shutdownButton->width = 75;
    shutdownButton->height = 40;
    shutdownButton->MouseClick += new StaticFunctionCallback(PoweroffButtonCallback);

    Button* shutdownAndEjectButton = new Button("Eject CD+Shutdown");
    shutdownAndEjectButton->width = 150;
    shutdownAndEjectButton->height = 30;
    shutdownAndEjectButton->y = 40;
    shutdownAndEjectButton->MouseClick += new StaticFunctionCallback(PoweroffAndEjectButtonCallback);

    Button* rebootButton = new Button("Reboot");
    rebootButton->width = 75;
    rebootButton->height = 40;
    rebootButton->x = 75;
    rebootButton->MouseClick += new StaticFunctionCallback(RebootButtonCallback);

    mainWindow->childs.push_back(shutdownButton);
    mainWindow->childs.push_back(shutdownAndEjectButton);
    mainWindow->childs.push_back(rebootButton);
    
    GUI::MakeAsync();
    while(GUI::HasItems())
        Process::Yield();

    return 0;
}
 
void PoweroffButtonCallback(void* sender, IEventArgs arg)
{
    DoSyscall(SYSCALL_SHUTDOWN);
}
void RebootButtonCallback(void* sender, IEventArgs arg)
{
    DoSyscall(SYSCALL_REBOOT);
}
void PoweroffAndEjectButtonCallback(void* sender, IEventArgs arg)
{
    Print("EjectDisk returned %d\n", (int)EjectDisk("B:\\"));
    DoSyscall(SYSCALL_SHUTDOWN);
}