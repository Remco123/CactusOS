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

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

int main()
{
    Context* screen = GUI::RequestContext(GUI::Width - 50, GUI::Height - 50, 25, 25);
    if(screen == 0)
            return -1;

    while(GUI::HasItems()) {
        unsigned int rgbColour[3];
        // Start off with red.
        rgbColour[0] = 255;
        rgbColour[1] = 0;
        rgbColour[2] = 0;  
        // Choose the colours to increment and decrement.
        for (int decColour = 0; decColour < 3; decColour += 1) {
            int incColour = decColour == 2 ? 0 : decColour + 1;
            // cross-fade the two colours.
            for(int i = 0; i < 255; i += 1) {
                rgbColour[decColour] -= 1;
                rgbColour[incColour] += 1;

                uint32_t col = 0xFF000000 | rgbColour[0] << 24 | rgbColour[1] << 8 | rgbColour[2];
                screen->canvas->Clear(col);
                Time::Sleep(5);
            }
        }

        Process::Yield();
    }
    
    return 0;
}