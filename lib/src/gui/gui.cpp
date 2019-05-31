#include <gui/gui.h>
#include <ipc.h>
#include <heap.h>
#include <syscall.h>
#include <log.h>

using namespace LIBCactusOS;

List<Control*>* GUI::Windows = 0;
int GUI::compositorPID = 3;
uint32_t GUI::curVirtualFramebufferAddress = 0xA0000000;

void GUI::Initialize()
{
    GUI::Windows = new List<Control*>();
}

bool GUI::ProcessEvents()
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
        Control* targetControl = FindTargetControl(guiEvent.arg2, guiEvent.arg3);
        if(targetControl == 0)
            return true;

        Print("Sending Mousedown to control %x\n", (uint32_t)targetControl);
        targetControl->OnMouseDown(guiEvent.arg2, guiEvent.arg3, guiEvent.arg4);
    }
    else if(guiEventType == EVENT_TYPE_MOUSEUP)
    {
        Control* targetControl = FindTargetControl(guiEvent.arg2, guiEvent.arg3);
        if(targetControl == 0)
            return true;

        Print("Sending Mouseup to control %x\n", (uint32_t)targetControl);
        targetControl->OnMouseUp(guiEvent.arg2, guiEvent.arg3, guiEvent.arg4);
    }
    return true;
}

Control* GUI::FindTargetControl(int mouseX, int mouseY)
{
    for(Control* c : *GUI::Windows)
    {
        if(mouseX >= c->x && mouseX <= c->x + c->width)
            if(mouseY >= c->y && mouseY <= c->y + c->height)
                return c;
    }
    return 0;
}

Canvas* GUI::RequestContext(int width, int height, int x, int y)
{
    if(IPCSend(compositorPID, IPC_TYPE_GUI, COMPOSITOR_REQUESTCONTEXT, curVirtualFramebufferAddress, width, height, x, y) != SYSCALL_RET_SUCCES)
        return 0;

    //Wait for response from server
    IPCMessage response = ICPReceive(compositorPID);
    if(response.type == IPC_TYPE_GUI && response.arg1 == 1) {
        uint32_t oldFB = curVirtualFramebufferAddress;
        uint32_t newFB = pageRoundUp(oldFB + width*height*4);
        
        curVirtualFramebufferAddress = newFB;
        return new Canvas((void*)oldFB, width, height);
    }
    
    return 0;
}