#ifndef __DEBUGGER_H
#define __DEBUGGER_H

#include "compositor.h"

class Compositor;

// Class used to display usefull debugging information when testing the compositor
class CompositorDebugger
{
private:
    // To which compositor is this debugger atached?
    Compositor* target = 0;
public:
    // Is the debugger enabled?
    bool enabled = false;

    // Create a new instance of a CompositorDebugger
    CompositorDebugger(Compositor* target);


    // Handle the debugging of one specific context
    // Called for every context every frame
    void ProcessContext(ContextInfo* ctx);

    // Handles any additional debugging for each frame
    void ProcessGeneral();
};

#endif