#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/widgets/inputbox.h>
#include <gui/directgui.h>
#include <string.h>
#include <log.h>
#include <proc.h>

void InputboxSubmit(void* sender, char* text)
{
    int childID = Process::Run(text);
    Process::BindSTDIO(childID, Process::ID);

    Print("Launched %s with id %d\n", text, childID);
}

int main()
{
    GUI::Initialize();

    Window* mainWindow = new Window(300, 200, WIDTH/2 - 150, HEIGHT/2 - 100);
    mainWindow->titleString = "CactusOS Terminal";

    Label* outputLabel = new Label();
    outputLabel->text = new char[1];
    outputLabel->text[0] = '\0';
    mainWindow->childs.push_back(outputLabel);

    Inputbox* input = new Inputbox();
    mainWindow->childs.push_back(input);
    input->y = mainWindow->height - input->height - 30;
    input->width = mainWindow->width - 2;
    input->x = 1;
    input->InputSubmit += InputboxSubmit;
    mainWindow->focusedChild = input;
    
    GUI::MakeAsync();
    while(GUI::HasItems())
    {
        char c = Process::ReadStdIn();
        outputLabel->text = str_Add(outputLabel->text, c);
    }

    return 0;
}