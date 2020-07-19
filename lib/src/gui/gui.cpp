#include <gui/gui.h>
#include <ipc.h>
#include <heap.h>
#include <syscall.h>
#include <log.h>
#include <proc.h>
#include <time.h>
#include <gui/contextinfo.h>
#include <gui/contextheap.h>
#include <gui/directgui.h>
#include <gui/fonts/fontparser.h>

using namespace LIBCactusOS;

int GUI::Width = 0;
int GUI::Height = 0;

List<Context*>* GUI::contextList = 0;
int GUI::compositorPID = 3;
Font* GUI::defaultFont = 0;

void GUI::Initialize()
{
    GUI::contextList = new List<Context*>();
    ContextHeap::Init();

    if(DoSyscall(SYSCALL_GET_SCREEN_PROPERTIES, (uint32_t)&GUI::Width, (uint32_t)&GUI::Height) == 0)
        Log(Error, "Error while requesting screen info");

    GUI::defaultFont = FontParser::FromFile("B:\\fonts\\Ubuntu14.cff");
}

void GUI::CleanUp()
{
    if(GUI::contextList != 0)
    {
        for(Context* c : *GUI::contextList)
        {
            c->CloseContext();
            delete c;
        }
        GUI:contextList->Clear();
    }
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
    else if(guiEventType == EVENT_TYPE_KEYPRESS)
    {
        // Find context that is currently focused and where the key should be send to.
        // TODO: Find a faster way for this, should be possible.
        for(int i = 0; i < contextList->size(); i++)
        {
            Context* c = contextList->GetAt(i);
            if(c->sharedContextInfo == 0)
                continue;

            if(c->sharedContextInfo->id == guiEvent.arg4)
            {
                KEYPACKET_FLAGS args = (KEYPACKET_FLAGS)guiEvent.arg3;
                if(args & Pressed)
                    c->OnKeyDown((uint8_t)guiEvent.arg2, args);
                else
                    c->OnKeyUp((uint8_t)guiEvent.arg2, args);
                break; // Quit loop
            }
        }
    }
    else if(guiEventType == EVENT_TYPE_RESIZE)
    {
        for(int i = 0; i < contextList->size(); i++) {
            Context* c = contextList->GetAt(i);
            if(c->sharedContextInfo == 0)
                continue;

            if(c->sharedContextInfo->id == guiEvent.arg2) {
                c->OnResize(Rectangle(guiEvent.arg3, guiEvent.arg4, guiEvent.arg5, guiEvent.arg6));
                break; // Quit loop
            }
        }
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
    uint32_t contextAddress = ContextHeap::AllocateArea(pageRoundUp(GUI::Width * GUI::Height * 4 + sizeof(ContextInfo)) / 0x1000);
    if(IPCSend(compositorPID, IPC_TYPE_GUI, COMPOSITOR_REQUESTCONTEXT, contextAddress, width, height, x, y) != SYSCALL_RET_SUCCES)
        return 0;

    //Wait for response from server
    IPCMessage response = ICPReceive(compositorPID, 0, IPC_TYPE_GUI);
    if(response.arg1 == 1) {
        //Create context struct
        Context* ret = new Context(contextAddress + sizeof(ContextInfo), width, height);
        ret->sharedContextInfo = (ContextInfo*)contextAddress;

        //Add it to our list
        contextList->push_front(ret);

        return ret;
    }
    
    return 0;
}

void AsyncGUILoop()
{
    while (1)
    {
        GUI::DrawGUI();
        if(IPCAvailible())
            GUI::ProcessEvents();
        else
            Process::Yield();
    }
}

void GUI::MakeAsync()
{
    Print("Creating GUI thread\n");
    Process::CreateThread(AsyncGUILoop, false);
}

bool GUI::HasItems()
{
    return (contextList->size() > 0);
}