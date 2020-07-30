#include <gui/widgets/button.h>
#include <log.h>

using namespace LIBCactusOS;

Button::Button(char* text)
: Control(80, 40)
{
    this->label = text;
    this->backColor = 0xFF190A39;
    this->borderColor = 0xFF479BFF;
    this->textAlignment = { Alignment::Horizontal::Center, Alignment::Vertical::Center };
}
void Button::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRoundedRect(this->backColor, x_abs, y_abs, this->width - 1, this->height - 1, 10);
    context->DrawRoundedRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1, 10);

    if(this->label != 0)
        Context::DrawStringAligned(context, this->font, this->label, 0xFF479BFF, *this, this->textAlignment, x_abs, y_abs - 2);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}
void Button::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    this->backColor = 0xFF606060;
    Control::OnMouseDown(x_abs, y_abs, button);
}
void Button::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    this->backColor = 0xFF190A39;
    Control::OnMouseUp(x_abs, y_abs, button);
}