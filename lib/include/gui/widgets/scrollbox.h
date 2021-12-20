#ifndef __LIBCACTUSOS__GUI__SCROLLBOX_H
#define __LIBCACTUSOS__GUI__SCROLLBOX_H

#include <gui/widgets/control.h>
#include <gui/widgets/scrollbar.h>

namespace LIBCactusOS
{
    class ScrollBox : public Control
    {
    public:
        ScrollBar* scrollVertical = 0;
        ScrollBar* scrollHorizontal = 0;
    public:
        ScrollBox();

        // Draw this scrollbox
        void DrawTo(Canvas* context, int x_abs, int y_abs) override;
    };
}

#endif