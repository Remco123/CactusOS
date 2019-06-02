#include <gui/widgets/window.h>
#include <gui/gui.h>
#include <log.h>

using namespace LIBCactusOS;

Window::Window(Context* parent, int w, int h, int x, int y)
: Control(w, h, x, y)
{
    parent->Windows.push_back(this);
}

void Window::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs + titleBarHeight, this->width, this->height - titleBarHeight - 1);
    context->DrawRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1);

    //Draw title bar
    context->DrawFillRect(this->titleBarColor, x_abs + 1, y_abs + 1, this->width - 1, this->titleBarHeight);
    context->DrawLine(this->borderColor, x_abs + 1, y_abs + this->titleBarHeight, x_abs + this->width, y_abs + titleBarHeight);

    if(this->titleString)
        context->DrawString(this->titleString, x_abs + 3, y_abs + 10, 0xFF000000);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y + titleBarHeight);
}

void Window::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    Print("Window %s has mouseDown\n", this->titleString);
    if(y_abs < this->titleBarHeight)
        this->titleBarColor = 0xFF1A7868;

    //Send event to children
    for(Control* c : this->childs)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y + this->titleBarHeight && y_abs <= c->y + c->height + this->titleBarHeight)
                c->OnMouseDown(x_abs - c->x, y_abs - c->y - this->titleBarHeight, button);
    }
}
void Window::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    Print("Window %s has mouseUp\n", this->titleString);
    if(y_abs < this->titleBarHeight)
        this->titleBarColor = 0xFF4CB272;

    //Send event to children
    for(Control* c : this->childs)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y + this->titleBarHeight && y_abs <= c->y + c->height + this->titleBarHeight)
                c->OnMouseUp(x_abs - c->x, y_abs - c->y - this->titleBarHeight, button);
    }
}