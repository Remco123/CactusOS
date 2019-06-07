#ifndef __LIBCACTUSOS__GUI__TEXTBOX_H
#define __LIBCACTUSOS__GUI__TEXTBOX_H

#include <gui/widgets/control.h>

namespace LIBCactusOS
{
    class Label : public Control
    {
    public:
        char* text = 0;

        /**
         * Create a new label with a peice of text
        */
        Label(char* text = 0);

        /**
         * Draw this label
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs);
    };
}

#endif