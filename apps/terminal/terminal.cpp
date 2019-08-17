#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/directgui.h>
#include <log.h>
#include <proc.h>

int main()
{
    GUI::Initialize();

    Window* mainWindow = new Window(300, 200, WIDTH/2 - 150, HEIGHT/2 - 100);

    Print("This text is send to stdout\n");
    Print("Stdin: %c\n", Process::ReadStdIn());
    
    GUI::MakeAsync();
    while(GUI::HasItems())
        Process::Yield();

    return 0;
}