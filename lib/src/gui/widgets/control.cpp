#include <gui/widgets/control.h>

using namespace LIBCactusOS;

Control::Control(int w, int h, int x, int y)
: Rectangle(w, h, x, y)
{
    this->childs.Clear();
}

Control::~Control()
{
    for(Control* c : this->childs)
        delete c;
    
    this->childs.Clear();
}

void Control::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height - 1);
    context->DrawRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}

void Control::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    //Send event to children
    for(Control* c : this->childs)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y && y_abs <= c->y + c->height)
                c->OnMouseDown(x_abs - c->x, y_abs - c->y, button);
    }
}
void Control::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    //Send event to children
    for(Control* c : this->childs)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y && y_abs <= c->y + c->height)
                c->OnMouseUp(x_abs - c->x, y_abs - c->y, button);
    }
}

void Control::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    //TODO: Implement mouseEnter and mouseLeave here
}
