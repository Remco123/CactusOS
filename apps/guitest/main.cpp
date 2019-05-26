#include <new.h>
#include <gui/guicom.h>
#include <log.h>
#include <time.h>
#include <string.h>
#include <gui/window.h>
#include <convert.h>

using namespace LIBCactusOS;

int main()
{
    Print("GUITest: Requesting context\n");

    uint32_t fb = GUICommunication::RequestContext(300, 200, 100, 100);
    if(fb == 0)
        return -1;

    Canvas gui((void*)fb, 300, 200);
    gui.Clear(0xFF77bb50);

    Window* window1 = new Window(300, 200, 0, 0);
    window1->titleString = "Window 1";
    window1->DrawTo(&gui, 0, 0);

    return 0;
}