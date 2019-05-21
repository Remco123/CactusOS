#include <new.h>
#include <gui/guicom.h>
#include <log.h>
#include <time.h>
#include <string.h>
#include <gui/canvas.h>

using namespace LIBCactusOS;

int main()
{
    Print("GUITest: Requesting context\n");

    GUICommunication::RequestContext(0xAB000000, 200, 200, 100, 100);

    Canvas gui((void*)0xAB000000, 200, 200);

    gui.Clear(0xFFAAAAAA);

    gui.DrawRect(0xFF000000, 0, 0, 199, 199);

    gui.DrawFillRect(0xFF0c69ff, 1, 1, 199, 30);

    gui.DrawLine(0xFF000000, 1, 30, 199, 30);

    return 0;
}