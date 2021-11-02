#include <gui/widgets/control.h>
#include <gui/gui.h>
#include <log.h>

using namespace LIBCactusOS;

Control::Control(int w, int h, int x, int y)
: EventObject(), Rectangle(w, h, x, y)
{
    this->childs.Clear();
    this->focusedChild = 0;
    this->parent = 0;
    this->anchor = Direction::Top | Direction::Left;
    this->font = GUI::defaultFont;
}

Control::~Control()
{
    for(Control* c : this->childs)
        delete c;
    
    this->childs.Clear();
}

void Control::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    Rectangle visual = Control::GetParentsBounds(x_abs, y_abs);
    if(this->cornerStyle == CornerStyle::Rounded) {
        context->DrawFillRoundedRect(this->backColor, visual.x, visual.y, visual.width, visual.height, this->cornerRadius);
        context->DrawRoundedRect(this->borderColor, visual.x, visual.y, visual.width, visual.height, this->cornerRadius);
    }
    else if(this->cornerStyle == CornerStyle::Sharp) {
        context->DrawFillRect(this->backColor, visual.x, visual.y, visual.width, visual.height);
        context->DrawRect(this->borderColor, visual.x, visual.y, visual.width, visual.height);
    }

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

Rectangle Control::GetParentsBounds(int xOffset, int yOffset)
{
    if(this->parent == 0)
        return Rectangle::Zero();
    
    Rectangle result;
    Rectangle source = *this;
    source.x = xOffset;
    source.y = yOffset;

    // Get the dimensions of the parent window/control
    Rectangle parentRect = *this->parent;
    parentRect.x = 0;
    parentRect.y = 0;
    
    // Check if the parent is a window, if so we need to take in account the title bar
    // TODO: Make title bar a control itself??
    if(this->parent == GUI::GetControlWindow(this)) {
        parentRect.y += ((Window*)this->parent)->titleBarHeight;
        parentRect.height -= ((Window*)this->parent)->titleBarHeight + 1;
    }
    
    // Calculate intersection
    if(parentRect.Intersect(source, &result))
        return result;
    
    return Rectangle::Zero();
}

void Control::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->MouseDown.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    // Send event to children
    for(Control* c : this->childs) {
        if(c->Contains(x_abs, y_abs)) {
            this->focusedChild = c;
            
            c->OnMouseDown(x_abs - c->x, y_abs - c->y, button);
        }
    }
}
void Control::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->MouseUp.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));
    this->MouseClick.Invoke(this, MouseButtonArgs(x_abs, y_abs, button));

    // Send event to children
    for(Control* c : this->childs) {
        if(c->Contains(x_abs, y_abs)) {
            c->OnMouseUp(x_abs - c->x, y_abs - c->y, button);
        }
    }
}

void Control::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    for(Control* c : this->childs) {
        bool inNewArea = c->Contains(newX_abs, newY_abs);
        bool inOldArea = c->Contains(prevX_abs, prevY_abs);

        if(inNewArea || inOldArea)
            c->OnMouseMove(prevX_abs - c->x, prevY_abs - c->y, newX_abs - c->x, newY_abs - c->y);

        if(inNewArea && !inOldArea)
            c->OnMouseEnter(prevX_abs - c->x, prevY_abs - c->y, newX_abs - c->x, newY_abs - c->y);
        
        if(!inNewArea && inOldArea)
            c->OnMouseLeave(prevX_abs - c->x, prevY_abs - c->y, newX_abs - c->x, newY_abs - c->y);
    }
}

void Control::OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{ }

void Control::OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{ }

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
void Control::ForcePaint()
{    
    Control* win = GUI::GetControlWindow(this);
    if(win)
        win->needsRepaint = true;
}
void Control::OnScroll(int32_t deltaZ, int x_abs, int y_abs)
{
    this->MouseScroll.Invoke(this, MouseScrollArgs(deltaZ, x_abs, y_abs));

    // Send event to children if they are in bounds
    for(Control* c : this->childs)
        if(c->Contains(x_abs, y_abs))
            c->OnScroll(deltaZ, x_abs - c->x, y_abs - c->y);
}