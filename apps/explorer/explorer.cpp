#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/directgui.h>
#include <log.h>
#include <time.h>
#include <proc.h>

int main()
{
    Window* mainWindow = new Window(150, 100, WIDTH/2 - 75, HEIGHT/2 - 50);
    mainWindow->titleString = "Resize Test";
    mainWindow->contextBase->sharedContextInfo->allowResize = true;
    
    GUI::MakeAsync();
    while(GUI::HasItems()) {
        Process::Yield();
    }

    return 0;
}