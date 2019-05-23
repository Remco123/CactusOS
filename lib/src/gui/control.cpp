#include <gui/control.h>

using namespace LIBCactusOS;

Control::Control(uint32_t w, uint32_t h)
: Control(width, height, 0, 0) { }

Control::Control(uint32_t w, uint32_t h, uint32_t x, uint32_t y)
: Rectangle(w, h, x, y)
{
    this->childs.Clear();
    this->parent = 0;
}

Control::~Control()
{
    for(Control* c : this->childs)
        delete c;
    
    this->childs.Clear();
}

void Control::DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height - 1);
    context->DrawRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}