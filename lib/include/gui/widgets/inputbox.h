#ifndef __LIBCACTUSOS__GUI__INPUTBOX_H
#define __LIBCACTUSOS__GUI__INPUTBOX_H

#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    class Inputbox : public Control
    {
    private:
        char* text;
    public:
        Inputbox();
        ~Inputbox();

        /**
         * Draw this inputbox
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs);

        EventHandlerList<char*> InputSubmit;
    /*/////////
    // Events
    *//////////
    friend class Window;
    friend class Context;
    protected:
        /**
         * Called on keypress
        */
        void OnKeyPress(char key) override;
    };
}

#endif