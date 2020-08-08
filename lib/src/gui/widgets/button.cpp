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
    this->textColor = 0xFF479BFF;
    this->cornerRadius = 10;
    this->cornerStyle = CornerStyle::Rounded;
}
void Button::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    if(this->cornerStyle == CornerStyle::Rounded) {
        context->DrawFillRoundedRect(this->backColor, x_abs, y_abs, this->width, this->height, this->cornerRadius);
        context->DrawRoundedRect(this->borderColor, x_abs, y_abs, this->width, this->height, this->cornerRadius);
    }
    else if(this->cornerStyle == CornerStyle::Sharp) {
        context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height);
        context->DrawRect(this->borderColor, x_abs, y_abs, this->width, this->height);
    }

    if(this->label != 0)
        Context::DrawStringAligned(context, this->font, this->label, this->textColor, *this, this->textAlignment, x_abs, y_abs - 2);

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