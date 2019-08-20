#include <gui/context.h>
#include <log.h>
#include <ipc.h>
#include <syscall.h>
#include <gui/gui.h>

using namespace LIBCactusOS;

Context::Context(uint32_t framebufferAddr, int width, int height)
{
    this->Window = 0;
    this->canvas = new Canvas((void*)framebufferAddr, width, height);
    this->sharedContextInfo = 0;
}

Context::~Context()
{
    if(this->canvas != 0)
        delete this->canvas;
}

void Context::DrawGUI()
{
    if(this->Window)
        this->Window->DrawTo(this->canvas, 0, 0);
}

void Context::MoveToPosition(int newX, int newY)
{
    int oldX = this->sharedContextInfo->x;
    int oldY = this->sharedContextInfo->y;

    this->sharedContextInfo->x = newX;
    this->sharedContextInfo->y = newY;

    if(this->Window)
    {
        this->Window->x = newX;
        this->Window->y = newY;
    }

    IPCSend(GUI::compositorPID, IPC_TYPE_GUI, COMPOSITOR_CONTEXTMOVED, oldX, oldY, this->sharedContextInfo->width, this->sharedContextInfo->height);
}

void Context::CloseContext()
{
    IPCSend(GUI::compositorPID, IPC_TYPE_GUI, COMPOSITOR_CONTEXTCLOSE, this->sharedContextInfo->id);
    GUI::contextList->Remove(this);
}

void Context::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->MouseDown.Invoke(this, MouseButtonArgs(button));

    if(this->Window == 0)
        return;

    Window->OnMouseDown(x_abs, y_abs, button);
}
void Context::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{    
    this->MouseUp.Invoke(this, MouseButtonArgs(button));
    this->MouseClick.Invoke(this, MouseButtonArgs(button));

    if(this->Window == 0)
        return;
    
    Window->OnMouseUp(x_abs, y_abs, button);
}
void Context::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    this->MouseMove.Invoke(this, MouseMoveArgs(prevX_abs, prevY_abs, newX_abs, newY_abs));

    if(this->Window == 0)
        return;
    
    Window->OnMouseMove(prevX_abs, prevY_abs, newX_abs, newY_abs);
}
void Context::OnKeyPress(char key)
{
    this->KeyPress.Invoke(this, KeypressArgs(key));
    
    Print("Context %x key press %c\n", (uint32_t)this, key);
}