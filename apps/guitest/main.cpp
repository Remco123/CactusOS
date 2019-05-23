#include <new.h>
#include <gui/guicom.h>
#include <log.h>
#include <time.h>
#include <string.h>
#include <gui/control.h>

using namespace LIBCactusOS;

int main()
{
    Print("GUITest: Requesting context\n");

    GUICommunication::RequestContext(0xAB000000, 200, 200, 100, 100);

    Canvas gui((void*)0xAB000000, 200, 200);
    gui.Clear(0xFF77bb50);

    Control* cntrl = new Control(200, 200, 100, 100);
    Control* cntrl2 = new Control(150, 70, 20, 15);

    cntrl->childs.push_back(cntrl2);
    cntrl->DrawTo(&gui, 0, 0);

    Time::Sleep(1000);

    cntrl2->backColor = 0xFF679901;
    cntrl->DrawTo(&gui, 0, 0);

    delete cntrl,cntrl2;

    return 0;
}