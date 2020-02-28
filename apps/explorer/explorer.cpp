#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/window.h>
#include <gui/directgui.h>
#include <log.h>
#include <time.h>
#include <proc.h>
#include <convert.h>
#include <string.h>
#include <vfs.h>

char* workingDirectory = "B:\\";
Control* menuBar = 0;
Control* leftBar = 0;
Control* fileArea = 0;
Button* refreshButton = 0;
bool shoudRefresh;
void LoadFilesAndDirectories();

void DirectoryItemButtonCallback(void* sender, MouseButtonArgs arg)
{
    Button* source = (Button*)sender;
    
    char* newPath = str_Combine(workingDirectory, source->label);

    Print("Loading %s\n", newPath);
    if(DirectoryExists(newPath)) {
        newPath = str_Combine(newPath, "\\");
        workingDirectory = newPath;
        Print("%s\n", workingDirectory);
        
        shoudRefresh = true;
    }
    else if(FileExists(newPath)) {

    }
    else
        Log(Error, "File or directory does not exist");
}

void LoadFilesAndDirectories()
{
    for(Control* child : fileArea->childs) {
        Button* but = (Button*)child;

        fileArea->RemoveChild(but);
        delete but->label;
        delete but;
    }

    auto dirList = DirectoryListing(workingDirectory);
    for(char* item : dirList) {
        Button* button = new Button(item);
        button->width = fileArea->width - 2;
        button->x = 1;
        button->anchor = (Left | Top | Right);
        button->y = button->height * fileArea->childs.size();
        button->MouseClick += DirectoryItemButtonCallback;
        fileArea->AddChild(button);
    }
}

void PartionButtonCallback(void* sender, MouseButtonArgs arg)
{
    Button* source = (Button*)sender;
    workingDirectory = source->label;
    LoadFilesAndDirectories();
}

void LoadPartitions()
{
    for(Control* child : leftBar->childs) {
        Button* but = (Button*)child;

        leftBar->RemoveChild(but);
        delete but->label;
        delete but;
    }

    int i = 0;
    bool run = true;
    while(run) {
        char* str = (char*)(i > 9 ? "  :\\" : " :\\");
        if(i < 10)
            str[0] = Convert::IntToString(i)[0];
        else {
            str[0] = Convert::IntToString(i)[0];
            str[1] = Convert::IntToString(i)[1];
        }
        run = DirectoryExists(str);
        i++;
    }
    i--;

    Print("Found %d Partitions \n", i);
    for(int c = 0; c < i; c++) {
        char* str = new char[5];
        if(c < 10) {
            memcpy(str, " :\\", 4);
            str[0] = Convert::IntToString(c)[0];
        }
        else {
            memcpy(str, "  :\\", 5);
            str[0] = Convert::IntToString(c)[0];
            str[1] = Convert::IntToString(c)[1];
        }


        Button* button = new Button(str);
        button->width = leftBar->width - 2;
        button->x = 1;
        button->anchor = (Left | Top | Right);
        button->y = button->height * c;
        button->MouseClick += PartionButtonCallback;
        leftBar->AddChild(button);
    }
}

void RefreshCallback(void* sender, MouseButtonArgs arg)
{
    LoadPartitions();
    LoadFilesAndDirectories();
}

int main()
{
    Window* mainWindow = new Window(500, 300, WIDTH/2 - 250, HEIGHT/2 - 150);
    mainWindow->titleString = "File Explorer";
    mainWindow->contextBase->sharedContextInfo->allowResize = true;

    menuBar = new Control(500-2, 30-1, 1, 1);
    menuBar->anchor = (Left | Top| Right);
    mainWindow->AddChild(menuBar);

    leftBar = new Control(100, 230, 1, 30);
    leftBar->anchor = (Left | Top | Bottom);
    mainWindow->AddChild(leftBar);

    fileArea = new Control(500-100-1, 230, 100, 30);
    fileArea->anchor = (Left | Top | Bottom | Right);
    mainWindow->AddChild(fileArea);

    refreshButton = new Button("Refresh");
    refreshButton->anchor = (Left | Top | Bottom);
    refreshButton->height = menuBar->height;
    refreshButton->MouseClick += RefreshCallback;
    menuBar->AddChild(refreshButton);

    LoadPartitions();
    LoadFilesAndDirectories();
    
    while(GUI::HasItems()) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
        if(shoudRefresh) {
            LoadFilesAndDirectories();
            shoudRefresh = false;
        }
    }

    return 0;
}