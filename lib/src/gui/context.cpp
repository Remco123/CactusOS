#include <gui/context.h>
#include <log.h>

using namespace LIBCactusOS;

Context::Context(uint32_t framebufferAddr, int width, int height, int x, int y)
: Rectangle(width, height, x, y)
{
    this->Window = 0;
    this->canvas = new Canvas((void*)framebufferAddr, width, height);
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

void Context::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    Print("Context::OnMouseDown(%d, %d)\n", x_abs, y_abs);
    if(this->Window == 0)
        return;

    Window->OnMouseDown(x_abs, y_abs, button);
}
void Context::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    if(this->Window == 0)
        return;
    
    Window->OnMouseUp(x_abs, y_abs, button);
}
void Context::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    Print("Context::OnMouseMove(%d,%d)\n", newX_abs, newY_abs);
}