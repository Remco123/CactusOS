#include <gui/context.h>
#include <log.h>

using namespace LIBCactusOS;

Context::Context(uint32_t framebufferAddr, int width, int height, int x, int y)
: Rectangle(width, height, x, y), Windows()
{
    this->Windows.Clear();
    this->canvas = new Canvas((void*)framebufferAddr, width, height);
}

Context::~Context()
{
    if(this->canvas != 0)
        delete this->canvas;
}

void Context::DrawGUI()
{
    for(Control* c : Windows)
        c->DrawTo(this->canvas, c->x, c->y);
}

void Context::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    for(Control* c : Windows)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y && y_abs <= c->y + c->height)
                c->OnMouseDown(x_abs - c->x, y_abs - c->y, button);
    }
}
void Context::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    for(Control* c : Windows)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y && y_abs <= c->y + c->height)
                c->OnMouseUp(x_abs - c->x, y_abs - c->y, button);       
    }
}