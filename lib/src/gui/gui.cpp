#include <gui/gui.h>
#include <ipc.h>
#include <heap.h>
#include <syscall.h>
#include <log.h>
#include <proc.h>
#include <time.h>

using namespace LIBCactusOS;

List<Context*>* GUI::contextList = 0;
int GUI::compositorPID = 3;
uint32_t GUI::curVirtualFramebufferAddress = 0xA0000000;

void GUI::Initialize()
{
    GUI::contextList = new List<Context*>();
}

void GUI::ProcessEvents()
{
    /*
    +--------+--------------------+
    | guiEvent Message           |
    +--------+--------------------+
    | type   | IPC_TYPE_GUI_EVENT |
    | source |   Message source   |
    | dest   |     That is us     |
    | arg1   | Type of gui event  |
    | arg2/6 |  Depends on event  |
    +--------+--------------------+
    */

    IPCMessage guiEvent = ICPReceive(compositorPID, 0, IPC_TYPE_GUI_EVENT);
    Print("GUI: Got event from compositor type=%d\n", guiEvent.arg1);
    
    int guiEventType = guiEvent.arg1;
    if(guiEventType == EVENT_TYPE_MOUSEDOWN)
    {
        Context* targetControl = FindTargetContext((int)guiEvent.arg2, (int)guiEvent.arg3);
        if(targetControl == 0)
            return;

        Print("Sending Mousedown to context %x\n", (uint32_t)targetControl);
        targetControl->OnMouseDown(guiEvent.arg2 - targetControl->x, guiEvent.arg3 - targetControl->y, guiEvent.arg4);
    }
    else if(guiEventType == EVENT_TYPE_MOUSEUP)
    {
        Context* targetControl = FindTargetContext((int)guiEvent.arg2, (int)guiEvent.arg3);
        if(targetControl == 0)
            return;

        Print("Sending Mouseup to context %x\n", (uint32_t)targetControl);
        targetControl->OnMouseUp(guiEvent.arg2 - targetControl->x, guiEvent.arg3 - targetControl->y, guiEvent.arg4);
    }
}

Context* GUI::FindTargetContext(int mouseX, int mouseY)
{
    if(contextList == 0)
        return 0;
    
    //Print("FindTargetContext x=%d y=%d\n", mouseX, mouseY);
    for(int i = 0; i < contextList->size(); i++)
    {
        Context* c = contextList->GetAt(i);
        //Print("%x x=%d y=%d w=%d h=%d\n", (uint32_t)c, c->x, c->y, c->width, c->height);
        if(mouseX >= c->x && mouseX <= c->x + c->width)
            if(mouseY >= c->y && mouseY <= c->y + c->height)
                return c;
    }
    return 0;
}

Context* GUI::RequestContext(int width, int height, int x, int y)
{
    if(IPCSend(compositorPID, IPC_TYPE_GUI, COMPOSITOR_REQUESTCONTEXT, curVirtualFramebufferAddress, width, height, x, y) != SYSCALL_RET_SUCCES)
        return 0;

    //Wait for response from server
    IPCMessage response = ICPReceive(compositorPID);
    if(response.type == IPC_TYPE_GUI && response.arg1 == 1) {
        uint32_t oldFB = curVirtualFramebufferAddress;
        uint32_t newFB = pageRoundUp(oldFB + width*height*4);
        
        curVirtualFramebufferAddress = newFB;

        //Create context struct
        Context* ret = new Context(oldFB, width, height, x, y);
        //Print("Created new context %x %d %d %d %d\n", oldFB, width, height, x, y);
        
        //Print("contextList %x\n", (uint32_t)contextList);
        //Add it to our list
        contextList->push_back(ret);

        return ret;
    }
    
    return 0;
}

void GUI::MainGUILoop()
{
    while(1) {
        Process::Yield();
    }
}