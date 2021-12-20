#ifndef __LIBCACTUSOS__GUI__SCROLLBAR_H
#define __LIBCACTUSOS__GUI__SCROLLBAR_H

#include <gui/widgets/control.h>
#include <gui/property.h>
#include <gui/events.h>
#include <gui/colors.h>

namespace LIBCactusOS
{
    // Different type of scrollbars
    enum ScrollBarType
    {
        Horizontal,
        Vertical
    };

    #define SCROLLBAR_DEFAULT_WIDTH  100
    #define SCROLLBAR_DEFAULT_HEIGHT 20

    // Represents an item where the user can drag a square to scroll something for example
    // Childs are not drawn for logic reasons
    class ScrollBar : public Control
    {
    public:
        // Type of this scrollbar
        GUIProperty<ScrollBarType> type = GUIProperty<ScrollBarType>(this, Vertical);
        
        // Minumum value, can be < 0
        GUIProperty<int> minValue = GUIProperty<int>(this, 0);
        
        // Maximum value
        GUIProperty<int> maxValue = GUIProperty<int>(this, 100);
        
        // Current value, usually changed by cursor
        GUIProperty<int> value = GUIProperty<int>(this, 0);
        
        // Size of field where user can drag with the mouse in pixels
        GUIProperty<int> dragSize = GUIProperty<int>(this, 20);

        // Color of dragbar
        GUIProperty<uint32_t> dragColor = GUIProperty<uint32_t>(this, 0xFF444444);

        // The impact of the scrollbar on the value parameter
        GUIProperty<double> scrollFactor = GUIProperty<double>(this, 2.0);
    public:
        // Create new scrollbar of some type
        ScrollBar(ScrollBarType type, int min = 0, int max = 100, int dragSize = 20);

        // Draw this scrollbar
        void DrawTo(Canvas* context, int x_abs, int y_abs) override;

        // Called when user scrolls mouse
        void OnScroll(int32_t deltaZ, int x_abs, int y_abs) override;

        // Called when mouse is down on this control
        void OnMouseDown(int x_abs, int y_abs, uint8_t button) override;

        // Called when mouse is up this control
        void OnMouseUp(int x_abs, int y_abs, uint8_t button) override;

        // Called when mouse is moved this control
        void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;
    };
}

#endif