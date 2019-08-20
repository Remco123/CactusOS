#include <gui/widgets/control.h>
#include <log.h>

using namespace LIBCactusOS;

Control::Control(int w, int h, int x, int y)
: EventObject(), Rectangle(w, h, x, y)
{
    this->childs.Clear();
    this->focusedChild = 0;
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
    this->MouseDown(EventArgs(this));

    //Send event to children
    for(Control* c : this->childs)
        if(c->Contains(x_abs, y_abs)) {
            this->focusedChild = c;
            c->OnMouseDown(x_abs - c->x, y_abs - c->y, button);
        }
}
void Control::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->MouseUp(EventArgs(this));
    this->MouseClick(EventArgs(this));

    //Send event to children
    for(Control* c : this->childs)
        if(c->Contains(x_abs, y_abs))
            c->OnMouseUp(x_abs - c->x, y_abs - c->y, button);
}

void Control::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    this->MouseMove(EventArgs(this));

    //TODO: Implement mouseEnter and mouseLeave here
}

void Control::OnKeyPress(char key)
{
    this->KeyPress(EventArgs(this));

    if(this->focusedChild != 0)
        this->focusedChild->OnKeyPress(key);
}
