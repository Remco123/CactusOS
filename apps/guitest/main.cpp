#include <new.h>
#include <log.h>
#include <time.h>
#include <string.h>
#include <gui/widgets/window.h>
#include <gui/widgets/button.h>
#include <convert.h>
#include <proc.h>
#include <gui/gui.h>

using namespace LIBCactusOS;

int main()
{    
    GUI::Initialize();

    Context* screen1 = GUI::RequestContext(300, 200, 120, 50);
    if(screen1 == 0)
        return -1;

    Window* window1 = new Window(screen1, 300, 200);
    window1->titleString = "Window 1";

    Button* but1 = new Button("Button 1");
    window1->childs.push_back(but1);

    while(1) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
        //Process::Yield();
    }

    return 0;
}