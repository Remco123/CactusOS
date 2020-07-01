#include <gui/widgets/control.h>
#include <log.h>

using namespace LIBCactusOS;

Control::Control(int w, int h, int x, int y)
: EventObject(), Rectangle(w, h, x, y)
{
    this->childs.Clear();
    this->focusedChild = 0;
    this->parent = 0;
    this->anchor = Top | Left;
}

Control::~Control()
{
    for(Control* c : this->childs)
        delete c;
    
    this->childs.Clear();
}

void Control::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(this->pBackColor, x_abs, y_abs, this->width, this->height - 1);
    context->DrawRect(this->pBorderColor, x_abs, y_abs, this->width - 1, this->height - 1);

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

void Control::OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers)
{
    this->KeyDown.Invoke(this, KeypressArgs(key, modifiers));

    if(this->focusedChild != 0)
        this->focusedChild->OnKeyDown(key, modifiers);
}
void Control::OnKeyUp(uint8_t key, KEYPACKET_FLAGS modifiers)
{
    this->KeyUp.Invoke(this, KeypressArgs(key, modifiers));

    if(this->focusedChild != 0)
        this->focusedChild->OnKeyUp(key, modifiers);
}

void Control::OnResize(Rectangle old)
{
    int dWidth = this->width - old.width;
    int dHeight = this->height - old.height;

    // Loop through childs and update their position
    for(Control* child : this->childs) {
        bool resized = false;
        Rectangle oldSize(child->width, child->height, child->x, child->y);
        if(child->anchor & Top) {
            if(child->anchor & Bottom) {
                child->height += dHeight;
                resized = true;
            }
        }
        if(child->anchor & Right) {
            if(child->anchor & Left) {
                child->width += dWidth;
                resized = true;
            }
            else { 
                child->x += dWidth;
            }
        }
        if(child->anchor & Bottom && !(child->anchor & Top)) {
            child->y += dHeight;
        }
        
        if(resized)
            child->OnResize(oldSize);

    }    
}
