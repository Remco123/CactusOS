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
    Rectangle visual = Control::GetParentsBounds(x_abs, y_abs);
    if(visual.Area() == 0)
        return; // No need to draw something if it isn't going to be visible anyway
    
    if(this->cornerStyle == CornerStyle::Rounded) {
        context->DrawFillRoundedRect(this->backColor, visual.x, visual.y, visual.width, visual.height, this->cornerRadius);
        context->DrawRoundedRect(this->borderColor, visual.x, visual.y, visual.width, visual.height, this->cornerRadius);
    }
    else if(this->cornerStyle == CornerStyle::Sharp) {
        context->DrawFillRect(this->backColor, visual.x, visual.y, visual.width, visual.height);
        context->DrawRect(this->borderColor, visual.x, visual.y, visual.width, visual.height);
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
void Button::OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{ }
void Button::OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs)
{ }