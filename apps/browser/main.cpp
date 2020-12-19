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

int main()
{
    GUI::SetDefaultFont();

    Window* mainWindow = new Window(600, 400, 300, 300);
    mainWindow->titleString = "CactusOS File Browser";

    Slider* slider1 = new Slider(0, 255);
    slider1->x = 100;
    slider1->y = 100;
    slider1->knobColor = Colors::Red;
    mainWindow->AddChild(slider1);

    Slider* slider2 = new Slider(0, 255);
    slider2->x = 100;
    slider2->y = 200;
    slider2->knobColor = Colors::Green;
    mainWindow->AddChild(slider2);


    Slider* slider3 = new Slider(0, 255);
    slider3->x = 100;
    slider3->y = 300;
    slider3->knobColor = Colors::Blue;
    mainWindow->AddChild(slider3);

    GUI::MakeAsync();
    while(GUI::HasItems())
    {
        mainWindow->backColor = Colors::FromARGB(255, slider1->position, slider2->position, slider3->position);
    }


    return 0;
}