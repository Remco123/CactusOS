#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include <ipc.h>
#include <time.h>
#include <math.h>
#include <gui/gui.h>
#include <imaging/image.h>
#include <gui/fonts/fontparser.h>
#include <gui/colors.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

char* msg = "Hello World!\nNewline is placed here\nSpecial characters: !@#$%^&*()-_=+,`~?<>";

int main()
{
    Context* screen = GUI::RequestContext(900, 300, 25, 25);
    if(screen == 0)
            return -1;
        
    screen->canvas->Clear(0xFFFFFFFF);

    Font* ubuntu = FontParser::FromFile("B:\\fonts\\Ubuntu14.cff");

    uint64_t t = Time::Ticks();
    
    screen->canvas->DrawString(ubuntu, msg, 5, 5, 0xFF000000);

    Print("Drawing strings took %f ms\n", Time::Ticks() - t);

    // Draw bounding box
    int w,h;
    ubuntu->BoundingBox(msg, &w, &h);
    screen->canvas->DrawRect(0xFF00FF00, 5, 5, w, h);

    while(GUI::HasItems()) {
        GUI::ProcessEvents();
        GUI::DrawGUI();
    }
    
    return 0;
}