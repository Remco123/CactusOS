#include <gui/gui.h>
#include <ipc.h>
#include <log.h>
#include <gui/guicom.h>

using namespace LIBCactusOS;

List<Control*>* GUI::Windows = 0;

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
    | dest   |   That is us       |
    | arg1   | Type of gui event  |
    | arg2/6 |  Depends on event  |
    +--------+--------------------+
    */

    IPCMessage guiEvent = ICPReceive(GUICommunication::windowServerID, 0, IPC_TYPE_GUI_EVENT);
    Print("GUI: Got event from winsvr type=%d arg2=%d\n", guiEvent.arg1, guiEvent.arg2);
    
    int guiEventType = guiEvent.arg1;
    if(guiEventType == EVENTTYPE_MOUSEDOWN)
    {
        Control* targetControl = FindTargetControl(guiEvent.arg2, guiEvent.arg3);
        if(targetControl == 0)
            return true;

        Print("Sending Mousedown to control %x\n", (uint32_t)targetControl);
        targetControl->OnMouseDown(guiEvent.arg2, guiEvent.arg3, guiEvent.arg4);
    }
    else if(guiEventType == EVENTTYPE_MOUSEUP)
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