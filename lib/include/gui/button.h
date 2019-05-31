#ifndef __LIBCACTUSOS__GUI__BUTTON_H
#define __LIBCACTUSOS__GUI__BUTTON_H

#include <gui/control.h>

namespace LIBCactusOS
{
    /**
     * A GUI button
    */
    class Button : public Control
    {
    private:
        char* label = 0;
    public:
        Button(char* text = 0);
        void DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs);

        /*/////////
        // Events
        *//////////
        void OnMouseDown(uint32_t x_abs, uint32_t y_abs, uint8_t button);
        void OnMouseUp(uint32_t x_abs, uint32_t y_abs, uint8_t button);

    };
}

#endif