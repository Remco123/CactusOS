#include <gui/widgets/slider.h>
#include <gui/widgets/window.h>
#include <log.h>
#include <math.h>

using namespace LIBCactusOS;

Slider::Slider(int min, int max, int current)
: Control(200, 15)
{
    this->minValue = min;
    this->maxValue = max;
    this->position = current;
}

void Slider::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    Rectangle visual = Control::GetParentsBounds(x_abs, y_abs);
    if(visual.Area() == 0)
        return; // No need to draw something if it isn't going to be visible anyway
    
    context->DrawFillRect(this->backColor, visual.x, visual.y, visual.width, visual.height);
    context->DrawRect(this->borderColor, visual.x, visual.y, visual.width, visual.height);
    
    double percent = (double)position / (double)(this->maxValue - this->minValue);
    context->DrawFillCircle(this->knobColor, x_abs + (int)((double)this->width * percent), y_abs + this->height/2, this->knobSize);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}

void Slider::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->knobColor -= 0x00333333;
    this->mouseDown = true;
}
void Slider::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->knobColor += 0x00333333;
    this->mouseDown = false;
}
void Slider::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    if(mouseDown) {
        this->position += ((double)(newX_abs - prevX_abs) / (double)this->width) * (this->maxValue - this->minValue);
        this->position = Math::Max(this->minValue, Math::Min(this->maxValue, this->position));
        this->OnValueChanged.Invoke(this, this->position);
    }
}
void Slider::OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    this->knobColor -= 0x00111111;
}

void Slider::OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    this->knobColor += 0x00111111;
}