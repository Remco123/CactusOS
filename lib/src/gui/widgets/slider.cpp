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
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height);
    context->DrawRect(this->borderColor, x_abs, y_abs, this->width, this->height);
    
    double percent = (double)position / (double)(this->maxValue - this->minValue);
    context->DrawFillCircle(Colors::AlphaBlend(this->knobColor, this->mouseDown ? 0x44000000 : Colors::Transparent), x_abs + (int)((double)this->width * percent), y_abs + this->height/2, this->knobSize);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}

void Slider::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->mouseDown = true;
}
void Slider::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->mouseDown = false;
}
void Slider::OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{
    if(mouseDown) {
        this->position += (newX_abs - prevX_abs);
        this->position = Math::Max(this->minValue, Math::Min(this->maxValue, this->position));
    }
}