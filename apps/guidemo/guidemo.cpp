#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include <ipc.h>
#include <time.h>
#include <math.h>
#include <gui/gui.h>
#include <imaging/image.h>
#include <gui/fonts/fontparser.h>
#include <gui/colors.h>
#include <gui/widgets/window.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

void TextAlignDemo(Window* win, char* str)
{
    Button* but1 = new Button(str);
    but1->width = 100; but1->height = 40;
    but1->x = 10; but1->y = 10;
    but1->textAlignment = { Alignment::Horizontal::Left, Alignment::Vertical::Top };
    win->AddChild(but1);

    Button* but2 = new Button(str);
    but2->width = 100; but2->height = 40;
    but2->x = 120; but2->y = 10;
    but2->textAlignment = { Alignment::Horizontal::Center, Alignment::Vertical::Top };
    win->AddChild(but2);

    Button* but3 = new Button(str);
    but3->width = 100; but3->height = 40;
    but3->x = 230; but3->y = 10;
    but3->textAlignment = { Alignment::Horizontal::Right, Alignment::Vertical::Top };
    win->AddChild(but3);



    Button* but4 = new Button(str);
    but4->width = 100; but4->height = 40;
    but4->x = 10; but4->y = 60;
    but4->textAlignment = { Alignment::Horizontal::Left, Alignment::Vertical::Center };
    win->AddChild(but4);

    Button* but5 = new Button(str);
    but5->width = 100; but5->height = 40;
    but5->x = 120; but5->y = 60;
    but5->textAlignment = { Alignment::Horizontal::Center, Alignment::Vertical::Center };
    win->AddChild(but5);

    Button* but6 = new Button(str);
    but6->width = 100; but6->height = 40;
    but6->x = 230; but6->y = 60;
    but6->textAlignment = { Alignment::Horizontal::Right, Alignment::Vertical::Center };
    win->AddChild(but6);


    Button* but7 = new Button(str);
    but7->width = 100; but7->height = 40;
    but7->x = 10; but7->y = 110;
    but7->textAlignment = { Alignment::Horizontal::Left, Alignment::Vertical::Bottom };
    win->AddChild(but7);

    Button* but8 = new Button(str);
    but8->width = 100; but8->height = 40;
    but8->x = 120; but8->y = 110;
    but8->textAlignment = { Alignment::Horizontal::Center, Alignment::Vertical::Bottom };
    win->AddChild(but8);

    Button* but9 = new Button(str);
    but9->width = 100; but9->height = 40;
    but9->x = 230; but9->y = 110;
    but9->textAlignment = { Alignment::Horizontal::Right, Alignment::Vertical::Bottom };
    win->AddChild(but9);
}

void DrawShapesDemo(Canvas* canv)
{
    int x = 350;
    int y = 50;
    int d = ((uint32_t)Time::Ticks() / 100);

    canv->DrawRect(Colors::Blue, x, y, 200, 200);
    canv->DrawRoundedRect(Colors::Red, x + 5, y + 5, 200 - 10, 200 - 10, d % 50);
    canv->DrawCircle(Colors::Green, x + 40, y + 40, d % 30);
    canv->DrawFillCircle(Colors::Green, x + 40, y + 40, d % 20);
    canv->DrawFillRoundedRect(Colors::Blue, x + 100, y + 20, 80, 80, d % 40);
}

int main()
{
    Window* mainWindow = new Window(600, 400, 300, 300);
    mainWindow->titleString = "CactusOS GUI Demo";
    
    TextAlignDemo(mainWindow, "Hello");

    while (GUI::HasItems()) {
        GUI::DrawGUI();
        DrawShapesDemo(mainWindow->contextBase->canvas);
        GUI::ProcessEvents();
    }
    
    return 0;
}