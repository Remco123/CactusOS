#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/button.h>
#include <gui/directgui.h>
#include <proc.h>
#include <syscall.h>
#include <vfs.h>
#include <log.h>

void PoweroffButtonCallback(void* sender, MouseButtonArgs arg);
void RebootButtonCallback(void* sender, MouseButtonArgs arg);
void PoweroffAndEjectButtonCallback(void* sender, MouseButtonArgs arg);

int main()
{
    Window* mainWindow = new Window(150, 100, GUI::Width/2 - 75, GUI::Width/2 - 50);
    mainWindow->titleString = "Power Options";

    Button* shutdownButton = new Button("Poweroff");
    shutdownButton->width = 75;
    shutdownButton->height = 40;
    shutdownButton->MouseClick += PoweroffButtonCallback;

    Button* shutdownAndEjectButton = new Button("Eject CD+Shutdown");
    shutdownAndEjectButton->width = 150;
    shutdownAndEjectButton->height = 30;
    shutdownAndEjectButton->y = 40;
    shutdownAndEjectButton->MouseClick += PoweroffAndEjectButtonCallback;

    Button* rebootButton = new Button("Reboot");
    rebootButton->width = 75;
    rebootButton->height = 40;
    rebootButton->x = 75;
    rebootButton->MouseClick += RebootButtonCallback;

    mainWindow->AddChild(shutdownButton);
    mainWindow->AddChild(shutdownAndEjectButton);
    mainWindow->AddChild(rebootButton);
    
    while(GUI::HasItems()) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    return 0;
}
 
void PoweroffButtonCallback(void* sender, MouseButtonArgs arg)
{
    DoSyscall(SYSCALL_SHUTDOWN);
}
void RebootButtonCallback(void* sender, MouseButtonArgs arg)
{
    DoSyscall(SYSCALL_REBOOT);
}
void PoweroffAndEjectButtonCallback(void* sender, MouseButtonArgs arg)
{
    Print("EjectDisk returned %d\n", (int)EjectDisk("B:\\"));
    DoSyscall(SYSCALL_SHUTDOWN);
}