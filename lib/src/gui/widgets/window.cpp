#include <gui/widgets/window.h>
#include <gui/gui.h>
#include <log.h>

using namespace LIBCactusOS;

Window::Window(Context* base)
: Control(base->sharedContextInfo->width, base->sharedContextInfo->height, base->sharedContextInfo->x, base->sharedContextInfo->y)
{
    base->Window = this;
    this->contextBase = base;
}

Window::Window(int width, int height, int x, int y)
: Control(width, height, x, y)
{
    Context* screen = GUI::RequestContext(width, height, x, y);
    if(screen == 0) {
        Log(Error, "Could not create a context for this window");
        return;
    }

    this->contextBase = screen;
    screen->Window = this;
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
    //Print("Window %s has mouseDown\n", this->titleString);
    if(y_abs < this->titleBarHeight) {
        titleBarMouseDown = true;
        mouseDownX = x_abs;
        mouseDownY = y_abs;
        this->titleBarColor = 0xFF1A7868;
    }

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
    //Print("Window %s has mouseUp\n", this->titleString);
    if(y_abs < this->titleBarHeight) {
        titleBarMouseDown = false;
        this->titleBarColor = 0xFF4CB272;
    }

    //Send event to children
    for(Control* c : this->childs)
    {
        if(x_abs >= c->x && x_abs <= c->x + c->width)
            if(y_abs >= c->y + this->titleBarHeight && y_abs <= c->y + c->height + this->titleBarHeight)
                c->OnMouseUp(x_abs - c->x, y_abs - c->y - this->titleBarHeight, button);
    }
}
void Window::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    if(this->titleBarMouseDown) {
        //Print("Window Drag!\n");
        this->contextBase->MoveToPosition(this->x + newX_abs - mouseDownX, this->y + newY_abs - mouseDownY);
    }
}