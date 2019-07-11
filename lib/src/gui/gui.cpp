#include <gui/gui.h>
#include <ipc.h>
#include <heap.h>
#include <syscall.h>
#include <log.h>
#include <proc.h>
#include <time.h>
#include <gui/contextinfo.h>

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
    //Print("GUI: Got event from compositor type=%d\n", guiEvent.arg1);
    
    int guiEventType = guiEvent.arg1;
    if(guiEventType == EVENT_TYPE_MOUSEDOWN)
    {
        Context* targetControl = FindTargetContext((int)guiEvent.arg2, (int)guiEvent.arg3);
        if(targetControl == 0)
            return;

        //Print("Sending Mousedown to context %x\n", (uint32_t)targetControl);
        targetControl->OnMouseDown(guiEvent.arg2 - targetControl->sharedContextInfo->x, guiEvent.arg3 - targetControl->sharedContextInfo->y, guiEvent.arg4);
    }
    else if(guiEventType == EVENT_TYPE_MOUSEUP)
    {
        Context* targetControl = FindTargetContext((int)guiEvent.arg2, (int)guiEvent.arg3);
        if(targetControl == 0)
            return;

        //Print("Sending Mouseup to context %x\n", (uint32_t)targetControl);
        targetControl->OnMouseUp(guiEvent.arg2 - targetControl->sharedContextInfo->x, guiEvent.arg3 - targetControl->sharedContextInfo->y, guiEvent.arg4);
    }
    else if(guiEventType == EVENT_TYPE_MOUSEMOVE)
    {
        int prevX = guiEvent.arg2;
        int prevY = guiEvent.arg3;
        int curX = guiEvent.arg4;
        int curY = guiEvent.arg5;

        Context* prevTargetControl = FindTargetContext(prevX, prevY);
        Context* newTargetControl = FindTargetContext(prevX, prevY);
        
        if(prevTargetControl != 0)
            prevTargetControl->OnMouseMove(prevX - prevTargetControl->sharedContextInfo->x, prevY - prevTargetControl->sharedContextInfo->y, curX - prevTargetControl->sharedContextInfo->x, curY - prevTargetControl->sharedContextInfo->y);
    
        if(newTargetControl != 0 && newTargetControl != prevTargetControl)
            newTargetControl->OnMouseMove(prevX - newTargetControl->sharedContextInfo->x, prevY - newTargetControl->sharedContextInfo->y, curX - newTargetControl->sharedContextInfo->x, curY - newTargetControl->sharedContextInfo->y);
    }
}

void GUI::DrawGUI()
{
    for(Context* c : *contextList)
        c->DrawGUI();
}

Context* GUI::FindTargetContext(int mouseX, int mouseY)
{
    if(contextList == 0)
        return 0;
    
    for(int i = 0; i < contextList->size(); i++)
    {
        Context* c = contextList->GetAt(i);
        if(c->sharedContextInfo == 0)
            continue;

        if(mouseX >= c->sharedContextInfo->x && mouseX <= c->sharedContextInfo->x + c->sharedContextInfo->width)
            if(mouseY >= c->sharedContextInfo->y && mouseY <= c->sharedContextInfo->y + c->sharedContextInfo->height)
                return c;
    }
    return 0;
}

Context* GUI::RequestContext(int width, int height, int x, int y)
{
    if(IPCSend(compositorPID, IPC_TYPE_GUI, COMPOSITOR_REQUESTCONTEXT, curVirtualFramebufferAddress, width, height, x, y) != SYSCALL_RET_SUCCES)
        return 0;

    //Wait for response from server
    IPCMessage response = ICPReceive(compositorPID, 0, IPC_TYPE_GUI);
    if(response.arg1 == 1) {
        uint32_t oldFB = curVirtualFramebufferAddress;
        uint32_t newFB = pageRoundUp(oldFB + width*height*4 + sizeof(ContextInfo));
        
        curVirtualFramebufferAddress = newFB;

        //Create context struct
        Context* ret = new Context(oldFB + sizeof(ContextInfo), width, height);
        ret->sharedContextInfo = (ContextInfo*)oldFB;

        //Add it to our list
        contextList->push_back(ret);

        return ret;
    }
    
    return 0;
}

void AsyncGUILoop()
{
    while (true)
    {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }
}

void GUI::MakeAsync()
{
    Print("Creating GUI thread\n");
    Process::CreateThread(AsyncGUILoop, false);
}