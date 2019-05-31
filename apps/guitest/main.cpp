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

    Canvas* canv = GUI::RequestContext(300, 200, 100, 100);
    if(canv == 0)
        return -1;

    Window* window1 = new Window(300, 200);
    window1->titleString = "Window 1";    
    window1->DrawTo(canv, 0, 0);

    while(GUI::ProcessEvents())
        window1->DrawTo(canv, 0, 0);

    return 0;
}