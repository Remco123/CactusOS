#include <new.h>
#include <gui/gui.h>
#include <gui/directgui.h>
#include <convert.h>
#include <time.h>
#include <datetime.h>
#include <string.h>
#include <math.h>
#include <log.h>
#include <proc.h>
#include <vfs.h>
#include "item.h"

List<DesktopItem*>* items = 0;

void ButtonHandler(void* sender, MouseButtonArgs arg)
{
    Print("Desktop Button Handler\n");
    Context* source = (Context*)sender;
    for(DesktopItem* item : *items)
        if(item->context == source && item->filename != 0)
            Process::Run(item->filename);
}

int main(int argc, char** argv)
{
    GUI::SetDefaultFont();

    // Create list
    items = new List<DesktopItem*>();
    
    Log(Info, "Parsing desktop items");
    if(FileExists("B:\\desktop\\items.txt") == false)
    {
        Log(Error, "B:\\desktop\\items.txt does not exist");
        return -1;
    }

    uint32_t fileSize = GetFileSize("B:\\desktop\\items.txt");
    if(fileSize != (uint32_t)-1)
    {
        char* fileBuf = new char[fileSize];
        ReadFile("B:\\desktop\\items.txt", (uint8_t*)fileBuf);

        int x = 5;
        int y = 5;
        
        List<char*> lines = str_Split(fileBuf, '\n');
        for(int i = 0; i < lines.size(); i++)
        {
            char* str = lines[i];
            if(str[0] != '"') { //not an entry
                delete str;
                continue;
            }

            int labelLength = str_IndexOf(str, '"', 1) - str_IndexOf(str, '"', 0) - 1;
            int filenameLength = str_IndexOf(str, '"', 3) - str_IndexOf(str, '"', 2) - 1;
            int iconpathLength = str_IndexOf(str, '"', 5) - str_IndexOf(str, '"', 4) - 1;

            char* labelBuffer = new char[labelLength+1];
            labelBuffer[labelLength] = '\0';
            char* filenameBuffer = new char[filenameLength+1];
            filenameBuffer[filenameLength] = '\0';
            char* iconpathBuffer = new char[iconpathLength+1];
            iconpathBuffer[iconpathLength] = '\0';

            memcpy(labelBuffer, str + str_IndexOf(str, '"', 0) + 1, labelLength);
            memcpy(filenameBuffer, str + str_IndexOf(str, '"', 2) + 1, filenameLength);
            memcpy(iconpathBuffer, str + str_IndexOf(str, '"', 4) + 1, iconpathLength);

            DesktopItem* item = new DesktopItem(x, y, 90, 110);
            x += 100;
            if(x >= 205)
            { x = 5; y += 120; }

            item->filename = filenameBuffer;
            item->label = labelBuffer;
            item->iconBuffer = 0;
            item->context->MouseClick += ButtonHandler;
            item->drawLabel = false;

            if(FileExists(iconpathBuffer))
            {
                uint32_t iconSize = GetFileSize(iconpathBuffer);
                if(iconSize != (uint32_t)-1)
                {
                    uint8_t* iconBuf = new uint8_t[iconSize];
                    ReadFile(iconpathBuffer, iconBuf);
                    item->iconBuffer = iconBuf;
                }
            }
            else
                item->drawLabel = true;
                
            items->push_back(item);
            item->DrawToContext();

            delete str;
        }
    }

    while(1)
    {
        GUI::ProcessEvents();
    }
}