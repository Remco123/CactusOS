#include <gui/button.h>
#include <log.h>

using namespace LIBCactusOS;

Button::Button(char* text)
: Control(80, 40)
{
    this->label = text;
    this->backColor = 0xFF898989;
}
void Button::DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height - 1);
    context->DrawRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1);
    if(this->label != 0)
        context->DrawString(this->label, x_abs + 3, y_abs + 6, 0xFF000000);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}
void Button::OnMouseDown(uint32_t x_abs, uint32_t y_abs, uint8_t button)
{
    Print("Button %d Mouse Down\n", button);

    this->backColor = 0xFF606060 + 0xFFFFF*button;

    Control::OnMouseDown(x_abs, y_abs, button);
}
void Button::OnMouseUp(uint32_t x_abs, uint32_t y_abs, uint8_t button)
{
    Print("Button %d Mouse Up\n", button);

    this->backColor = 0xFF898998;

    Control::OnMouseUp(x_abs, y_abs, button);
}