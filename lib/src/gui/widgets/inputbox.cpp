#include <gui/widgets/inputbox.h>
#include <string.h>
#include <log.h>

using namespace LIBCactusOS;

Inputbox::Inputbox()
: Control(200, 20), InputSubmit() {
    this->text = new char[1];
    this->text[0] = '\0';

    this->backColor = 0xFFFFFFFF;
    this->borderColor = 0xFF000000;
}
Inputbox::~Inputbox()
{
    if(this->text != 0)
        delete this->text;
}

void Inputbox::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(this->backColor, x_abs, y_abs, this->width, this->height - 1);

    if(this->Focused() == false) {
        context->DrawRect(this->borderColor, x_abs, y_abs, this->width - 1, this->height - 1);   
    }
    else {
        context->DrawRect(0xFF0000FF, x_abs, y_abs, this->width - 1, this->height - 1);
    }
    
    context->DrawString(this->font, this->text, x_abs + 2, y_abs + 5, 0xFF000000);
}

void Inputbox::OnKeyDown(uint8_t key, KEYPACKET_FLAGS modifiers)
{
    switch(key)
    {
        case '\n':
            {
                InputSubmit.Invoke(this, this->text); //Fire submit event
        
                // And reset text
                delete this->text;
                this->text = new char[1];
                this->text[0] = '\0';
            }
            break;
        case '\b':
            {
                int len = strlen(this->text);
                if(len > 0)
                    this->text[len-1] = '\0';
            }
            break;
        case '\t':
            {
                for(int i = 0; i < 4; i++)
                    this->text = str_Add(this->text, ' ');
            }
            break;
        default:
            {
                if(isvalid(key))
                    this->text = str_Add(this->text, key);
            }
            break;
    }

    Control::OnKeyDown(key, modifiers);
}