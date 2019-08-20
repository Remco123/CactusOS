#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/directgui.h>
#include <string.h>
#include <log.h>
#include <proc.h>

int main()
{
    GUI::Initialize();

    Window* mainWindow = new Window(300, 200, WIDTH/2 - 150, HEIGHT/2 - 100);
    mainWindow->titleString = "CactusOS Terminal";

    Label* outputLabel = new Label();
    outputLabel->text = new char[1];
    outputLabel->text[0] = '\0';
    mainWindow->childs.push_back(outputLabel);

    int childID = Process::Run("B:\\apps\\echo.bin");
    Process::BindSTDIO(childID, Process::ID);
    
    GUI::MakeAsync();
    while(GUI::HasItems())
    {
        char c = Process::ReadStdIn();
        outputLabel->text = str_Add(outputLabel->text, c);
    }

    return 0;
}