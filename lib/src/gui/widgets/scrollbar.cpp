#include <gui/widgets/scrollbar.h>
#include <math.h>

using namespace LIBCactusOS;

ScrollBar::ScrollBar(ScrollBarType type, int min, int max, int dragSize)
: Control(type == Horizontal ? SCROLLBAR_DEFAULT_WIDTH : SCROLLBAR_DEFAULT_HEIGHT, type == Vertical ? SCROLLBAR_DEFAULT_WIDTH : SCROLLBAR_DEFAULT_HEIGHT)
{ }

void ScrollBar::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    Rectangle visual = Control::GetParentsBounds(x_abs, y_abs);
    if(visual.Area() == 0)
        return; // No need to draw something if it isn't going to be visible anyway

    // Calculate percentage of completion
    double factor = (double)Math::Constrain(this->value, this->minValue, this->maxValue) / (double)(this->maxValue - this->minValue);

    context->DrawFillRect(this->backColor, visual.x, visual.y, visual.width, visual.height);
    if(this->type == Horizontal)
        context->DrawFillRect(this->dragColor, visual.x + (double)(this->width - this->dragSize) * factor, visual.y, this->dragSize, this->height);
    else
        context->DrawFillRect(this->dragColor, visual.x, visual.y + (double)(this->height - this->dragSize) * factor, this->width, this->dragSize);

    context->DrawRect(this->borderColor, visual.x, visual.y, visual.width, visual.height);
}

void ScrollBar::OnScroll(int32_t deltaZ, int x_abs, int y_abs)
{
    this->value = Math::Constrain(this->value + ((double)deltaZ * this->scrollFactor), this->minValue, this->maxValue);
}


void ScrollBar::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    
}

void ScrollBar::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{

}

void ScrollBar::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{

}