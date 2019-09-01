#include <gui/widgets/control.h>
#include <log.h>

using namespace LIBCactusOS;

Control::Control(int w, int h, int x, int y)
: EventObject(), Rectangle(w, h, x, y)
{
    this->childs.Clear();
    this->focusedChild = 0;
    this->parent = 0;
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

void Control::AddChild(Control* child, bool focus)
{
    this->childs += child;
    if(focus)
        this->focusedChild = child;
    
    child->parent = this;
}

void Control::RemoveChild(Control* child)
{
    this->childs -= child;
    if(this->focusedChild == child && this->childs.size() > 0)
        this->focusedChild = this->childs[this->childs.size() - 1]; //Last entry in child list
    
    child->parent = 0;
}

bool Control::Focused()
{
    return (this->parent != 0 && this->parent->focusedChild == this);
}

void Control::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->MouseDown.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    //Send event to children
    for(Control* c : this->childs)
        if(c->Contains(x_abs, y_abs)) {
            this->focusedChild = c;
            //Print("Foccused is now -> %x (%d,%d,%d,%d)\n", (uint32_t)c, c->width, c->height, c->x, c->y);
            c->OnMouseDown(x_abs - c->x, y_abs - c->y, button);
        }
}
void Control::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->MouseUp.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));
    this->MouseClick.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    //Send event to children
    for(Control* c : this->childs)
        if(c->Contains(x_abs, y_abs))
            c->OnMouseUp(x_abs - c->x, y_abs - c->y, button);
}

void Control::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    //TODO: Implement mouseEnter and mouseLeave here
}

void Control::OnKeyPress(char key)
{
    this->KeyPress.Invoke(this, KeypressArgs(key));

    if(this->focusedChild != 0)
        this->focusedChild->OnKeyPress(key);
}
