#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/button.h>
#include <gui/widgets/control.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/widgets/slider.h>
#include <gui/directgui.h>
#include <convert.h>
#include <string.h>
#include <log.h>
#include <proc.h>

using namespace LIBCactusOS;

Window* mainWindow = 0;
Slider* slider1 = 0;
Slider* slider2 = 0;
Slider* slider3 = 0;
Slider* slider4 = 0;

class DragControl : public Control
{
private:
    bool mouseDown = false;
    int mouseDownX = 0;
    int mouseDownY = 0;
public:
    DragControl(int w, int h, int x = 0, int y = 0)
    : Control(w,h,x,y) {}
    void OnMouseDown(int x_abs, int y_abs, uint8_t button) override
    {
        this->mouseDown = true;
        this->mouseDownX = x_abs;
        this->mouseDownY = y_abs;
    }
    void OnMouseUp(int x_abs, int y_abs, uint8_t button) override
    {
        this->mouseDown = false;
    }
    void OnMouseMove(int prevX_abs, int prevY_abs, int newX_abs, int newY_abs) override
    {
        if(this->mouseDown) {
            this->x = this->x + newX_abs - mouseDownX;
            this->y = this->y + newY_abs - mouseDownY;
            this->ForcePaint();
        }
    }
    void DrawTo(Canvas* context, int x_abs, int y_abs) override
    {
        Rectangle visual = this->GetParentsBounds();
        // Print("Visual = (%d, %d)   :   Abs = (%d, %d)\n", visual.x, visual.y, x_abs, y_abs);
        context->DrawFillRect(this->backColor, visual.x, visual.y, visual.width, visual.height);
        context->DrawRect(this->borderColor, visual.x, visual.y, visual.width, visual.height);
    }
};

void SliderChanged(void* sender, int newValue)
{
    mainWindow->backColor = Colors::FromARGB(slider4->position, slider1->position, slider2->position, slider3->position);
}

int main(int argc, char** argv)
{
    GUI::SetDefaultFont();

    mainWindow = new Window(600, 600, 300, 300);
    mainWindow->titleString = "CactusOS File Browser";

    slider1 = new Slider(0, 255, 255/2);
    slider1->x = 100;
    slider1->y = 100;
    slider1->knobColor = Colors::Red;
    slider1->OnValueChanged += SliderChanged;
    mainWindow->AddChild(slider1);

    slider2 = new Slider(0, 255, 255/2);
    slider2->x = 100;
    slider2->y = 200;
    slider2->knobColor = Colors::Green;
    slider2->OnValueChanged += SliderChanged;
    mainWindow->AddChild(slider2);

    slider3 = new Slider(0, 255, 255/2);
    slider3->x = 100;
    slider3->y = 300;
    slider3->knobColor = Colors::Blue;
    slider3->OnValueChanged += SliderChanged;
    mainWindow->AddChild(slider3);

    slider4 = new Slider(0, 255, 255/2);
    slider4->x = 100;
    slider4->y = 400;
    slider4->knobColor = Colors::White;
    slider4->OnValueChanged += SliderChanged;
    mainWindow->AddChild(slider4);

    DragControl* drag = new DragControl(150, 150, 50, 50);
    drag->backColor = Colors::Red;
    mainWindow->AddChild(drag);

    mainWindow->contextBase->sharedContextInfo->supportsTransparency = true;

    SliderChanged(0, 0);
    while (GUI::HasItems()) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    return 0;
}