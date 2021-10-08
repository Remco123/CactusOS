#include <gui/context.h>
#include <log.h>
#include <ipc.h>
#include <syscall.h>
#include <gui/gui.h>
#include <gui/contextheap.h>
#include <heap.h>

using namespace LIBCactusOS;

Context::Context(uint32_t framebufferAddr, int width, int height)
{
    this->Window = 0;
    this->canvas = new Canvas((void*)framebufferAddr, width, height);
    this->sharedContextInfo = 0;
}

void Context::DrawGUI()
{
    if(this->Window && this->Window->needsRepaint) {
        this->Window->DrawTo(this->canvas, 0, 0);
        this->Window->needsRepaint = false;
        this->sharedContextInfo->AddDirtyArea(0, 0, this->Window->width, this->Window->height);
    }
}

void Context::DrawStringAligned(Canvas* target, Font* font, char* string, uint32_t color, Rectangle bounds, Alignment align, int xoff, int yoff)
{
    if(target == 0 || font == 0 || string == 0)
        return;
    
    Rectangle textBounds;
    font->BoundingBox(string, &textBounds.width, &textBounds.height);

    int x = 0;
    int y = 0;
    
    switch(align.x) {
        case Alignment::Horizontal::Left:
            x = 0;
            break;
        case Alignment::Horizontal::Center:
            x = bounds.width/2 - textBounds.width/2;
            break;
        case Alignment::Horizontal::Right:
            x = bounds.width - textBounds.width;
            break;
    }
    switch(align.y) {
        case Alignment::Vertical::Top:
            y = 0;
            break;
        case Alignment::Vertical::Center:
            y = bounds.height/2 - textBounds.height/2;
            break;
        case Alignment::Vertical::Bottom:
            y = bounds.height - textBounds.height;
            break;
    }

    target->DrawString(font, string, x + xoff, y + yoff, color);
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

    IPCSend(GUI::compositorPID, IPCMessageType::GUIRequest, GUICommunction::CONTEXT_MOVED, oldX, oldY, this->sharedContextInfo->width, this->sharedContextInfo->height);
    this->sharedContextInfo->AddDirtyArea(0, 0, this->sharedContextInfo->width, this->sharedContextInfo->height);
}

void Context::CloseContext()
{
    ContextHeap::FreeArea(this->sharedContextInfo->virtAddrClient - CONTEXT_INFO_SIZE, pageRoundUp(this->sharedContextInfo->bytes) / 0x1000);
    GUI::contextList->Remove(this);
    if(this->canvas != 0)
        delete this->canvas;

    IPCSend(GUI::compositorPID, IPCMessageType::GUIRequest, GUICommunction::REQUEST_CLOSE, this->sharedContextInfo->id);
    if(ICPReceive(GUI::compositorPID, 0, IPCMessageType::GUIRequest).arg1 != 1)
        Log(Error, "Did not receive ack from compositor when removing context");
}

void Context::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->MouseDown.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    if(this->Window == 0)
        return;

    Window->OnMouseDown(x_abs, y_abs, button);
}
void Context::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{    
    this->MouseUp.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));
    this->MouseClick.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    if(this->Window == 0)
        return;
    
    Window->OnMouseUp(x_abs, y_abs, button);
}
void Context::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    if(this->Window == 0)
        return;
    
    Window->OnMouseMove(prevX_abs, prevY_abs, newX_abs, newY_abs);
}
void Context::OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers)
{
    this->KeyDown.Invoke(this, KeypressArgs(key, modifiers));
    
    if(this->Window == 0)
        return;
    
    Window->OnKeyDown(key, modifiers);
}
void Context::OnKeyUp(uint8_t key, KEYPACKET_FLAGS modifiers)
{
    this->KeyUp.Invoke(this, KeypressArgs(key, modifiers));
    
    if(this->Window == 0)
        return;
    
    Window->OnKeyUp(key, modifiers);
}
void Context::OnResize(Rectangle oldSize)
{
    this->canvas->Width = this->sharedContextInfo->width;
    this->canvas->Height = this->sharedContextInfo->height;
    
    if(this->Window == 0 || this->sharedContextInfo == 0)
        return;
    
    Window->x = this->sharedContextInfo->x;
    Window->y = this->sharedContextInfo->y;
    Window->width = this->sharedContextInfo->width;
    Window->height = this->sharedContextInfo->height;
    Window->OnResize(oldSize);
}
void Context::OnScroll(int32_t deltaZ, int x_abs, int y_abs)
{
    this->MouseScroll.Invoke(this, MouseScrollArgs(deltaZ, x_abs, y_abs));
    
    if(this->Window == 0)
        return;
    
    Window->OnScroll(deltaZ, x_abs, y_abs);
}