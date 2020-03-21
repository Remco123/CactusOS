#include <gui/widgets/button.h>
#include <log.h>

using namespace LIBCactusOS;

Button::Button(char* text)
: Control(80, 40)
{
    this->label = text;
    this->backColor = 0xFF898989;
}
void Button::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height - 1);
    for(uint32_t i = 0; i < 3; i++)
        context->DrawRect(this->borderColor + i*0x00333333, x_abs + i, y_abs + i, this->width - 1 - i*2, this->height - 1 - i*2);
    
    if(this->label != 0)
        context->DrawString(this->label, x_abs + 3, y_abs + 6, 0xFF000000);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}
void Button::OnMouseDown(int x_abs, int y_abs, uint8_t button)
{
    //Print("Button %d Mouse Down\n", button);

    this->backColor = 0xFF606060 + 0xFFFFF*button;

    Control::OnMouseDown(x_abs, y_abs, button);
}
void Button::OnMouseUp(int x_abs, int y_abs, uint8_t button)
{
    //Print("Button %d Mouse Up\n", button);
    this->backColor = 0xFF898989;

    Control::OnMouseUp(x_abs, y_abs, button);
}