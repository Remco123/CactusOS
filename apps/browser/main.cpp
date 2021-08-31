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

void SliderChanged(void* sender, int newValue)
{
    mainWindow->backColor = Colors::FromARGB(255, slider1->position, slider2->position, slider3->position);
}

int main(int argc, char** argv)
{
    GUI::SetDefaultFont();

    mainWindow = new Window(600, 400, 300, 300);
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

    SliderChanged(0, 0);
    while (GUI::HasItems()) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    return 0;
}