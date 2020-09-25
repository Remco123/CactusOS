#include <gui/widgets/label.h>

using namespace LIBCactusOS;

Label::Label(char* text)
: Control(80, 20)
{
    this->text = text;
}

void Label::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    if(this->text)
        context->DrawString(this->font, this->text, x_abs + 2, y_abs + 2, this->textColor);

    for(Control* c : this->childs)
        c->DrawTo(context, x_abs + c->x, y_abs + c->y);
}