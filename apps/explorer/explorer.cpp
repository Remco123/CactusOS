#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/directgui.h>
#include <log.h>
#include <time.h>
#include <proc.h>
#include <convert.h>
#include <string.h>

int main()
{
    Window* mainWindow = new Window(150, 100, WIDTH/2 - 75, HEIGHT/2 - 50);
    mainWindow->titleString = "Resize Test";
    mainWindow->contextBase->sharedContextInfo->allowResize = true;

    Control* c1 = new Control(130, 50, 10, 10);
    c1->anchor = (Left | Top | Right | Bottom);
    mainWindow->AddChild(c1);

    Control* c2 = new Control(80, 30, 10, 10);
    c2->anchor = (Right | Bottom);
    c1->AddChild(c2);
    
    GUI::MakeAsync();
    while(GUI::HasItems()) {
        Process::Yield();
    }

    return 0;
}