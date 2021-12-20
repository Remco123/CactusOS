#ifndef __LIBCACTUSOS__GUI__LISTVIEW_H
#define __LIBCACTUSOS__GUI__LISTVIEW_H

#include <gui/widgets/control.h>
#include <imaging/image.h>
#include <gui/gui.h>

namespace LIBCactusOS
{
    class ListViewItem
    {
    public:
        char* title;
        Imaging::Image* icon;
    public:
        ListViewItem();
        ~ListViewItem();
    };

    class ListView : public Control
    {
    private:
        List<ListViewItem> items;
    public:
        ListView();
        ~ListView();

        /**
         * Draw this button
        */
        void DrawTo(Canvas* context, int x_abs, int y_abs) override;

    /*/////////
    // Events
    *//////////
    friend class Window;
    friend class Context;
    protected:
        /**
         * Called when mouse is down on control
        */
        void OnMouseDown(int x_abs, int y_abs, uint8_t button) override;
        /**
         * Called when mouse is up on control
        */
        void OnMouseUp(int x_abs, int y_abs, uint8_t button) override;
    };
}

#endif