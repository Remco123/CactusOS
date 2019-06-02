#include <new.h>
#include <log.h>
#include <time.h>
#include <string.h>
#include <gui/widgets/window.h>
#include <convert.h>
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

    
    
    Context* screen2 = GUI::RequestContext(300, 200, 520, 200);
    if(screen2 == 0)
        return -1;

    Window* window2 = new Window(screen2, 300, 200);
    window2->titleString = "Window 2";

    while(1) {
        window1->DrawTo(screen1->canvas, 0, 0);
        window2->DrawTo(screen2->canvas, 0, 0);
        GUI::ProcessEvents();
    }

    return 0;
}