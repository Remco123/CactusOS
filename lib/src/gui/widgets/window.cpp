#include <gui/widgets/window.h>
#include <gui/gui.h>
#include <log.h>

using namespace LIBCactusOS;

Window::Window(uint32_t w, uint32_t h, uint32_t x, uint32_t y)
: Control(w, h, x, y)
{
    GUI::Windows->push_back(this);
}

void Window::DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs)
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

void Window::OnMouseDown(uint32_t x_abs, uint32_t y_abs, uint8_t button)
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
void Window::OnMouseUp(uint32_t x_abs, uint32_t y_abs, uint8_t button)
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