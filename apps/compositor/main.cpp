#include <new.h>
#include <log.h>
#include <time.h>
#include <proc.h>
#include <gui/directgui.h>

#include "compositor.h"

using namespace LIBCactusOS;

int main()
{
    Compositor* mainCompositor = new Compositor();
    while(1) 
    {
        uint32_t startTime = Time::Ticks();

        // Update cursor position with the value stored in SysInfo
        mainCompositor->curMouseX = Process::systemInfo->MouseX;
        mainCompositor->curMouseY = Process::systemInfo->MouseY;

        ///////////////////////////
        // Process GUI Events
        ///////////////////////////
        mainCompositor->ProcessEvents();

        ///////////////////////////
        // Process GUI Requests from clients
        ///////////////////////////
        mainCompositor->ProcessRequests();

        ///////////////////////////
        // Draw a new version of the desktop
        ///////////////////////////
        mainCompositor->DrawFrame();

        // Update cursor variables for next run
        mainCompositor->prevMouseX = mainCompositor->curMouseX;
        mainCompositor->prevMouseY = mainCompositor->curMouseY;

        //Print("Frametime = %d ms\n", Time::Ticks() - startTime);

        // Switch processes after drawing desktop
        // It is useless to draw it like 30 times in a couple milliseconds.
        Process::Yield();
    }

    delete mainCompositor;
    return 0;
}