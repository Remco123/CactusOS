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
    Label* outputLabel = new Label("Waiting...\n");
    mainWindow->childs.push_back(outputLabel);

    int childID = Process::Run("B:\\apps\\echo.bin");
    Process::BindSTDIO(childID, Process::ID);
    
    GUI::MakeAsync();
    while(GUI::HasItems())
    {
        char c = Process::ReadStdIn();
        
        int oldLen = strlen(outputLabel->text);
        char* newStr = new char[oldLen + 1];
        memcpy(newStr, outputLabel->text, oldLen);
        newStr[oldLen] = c;
        newStr[oldLen + 1] = '\0';
        //delete outputLabel->text;

        outputLabel->text = newStr;
    }

    return 0;
}