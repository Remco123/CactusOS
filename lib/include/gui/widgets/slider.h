#ifndef __LIBCACTUSOS__GUI__SLIDER_H
#define __LIBCACTUSOS__GUI__SLIDER_H

#include <gui/widgets/control.h>
#include <gui/property.h>
#include <gui/events.h>
#include <gui/colors.h>

namespace LIBCactusOS
{
    class Slider : public Control
    {
    private:
        bool mouseDown = false;
    public:
        GUIProperty<int> minValue = GUIProperty<int>(this, 0);
        GUIProperty<int> maxValue = GUIProperty<int>(this, 0);
        GUIProperty<int> position = GUIProperty<int>(this, 0);
        GUIProperty<uint32_t> knobColor = GUIProperty<uint32_t>(this, Colors::Blue);
        GUIProperty<int> knobSize = GUIProperty<int>(this, 10);

        EventHandlerList<int> OnValueChanged;
    public:    
        Slider(int min = 0, int max = 100, int current = 50);

        // Called when mouse is down on this control
        void OnMouseDown(int x_abs, int y_abs, uint8_t button) override;

        // Called when mouse is up this control
        void OnMouseUp(int x_abs, int y_abs, uint8_t button) override;

        // Called when mouse is moved this control
        void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;

        // Called when mouse enters control
        void OnMouseEnter(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;
        
        // Called when mouse leaves control
        void OnMouseLeave(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override;

        // Draw this slider
        void DrawTo(Canvas* context, int x_abs, int y_abs) override;
    };
}

#endif