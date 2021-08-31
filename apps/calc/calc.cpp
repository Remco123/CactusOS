#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/button.h>
#include <gui/widgets/control.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <gui/directgui.h>
#include <convert.h>
#include <string.h>
#include <log.h>
#include <proc.h>

using namespace LIBCactusOS;

void ButtonClickHandler(void* sender, MouseButtonArgs arg);
void CalculateButtonHandler(void* sender, MouseButtonArgs arg);
void NewClickHandler(void* sender, MouseButtonArgs arg)
{
    Process::Run("B:\\apps\\calc.bin");
}

Label* textLabel = 0;
bool calculated = true;
int main(int argc, char** argv)
{
    Print("Starting Calculator\n");
    GUI::SetDefaultFont();

    Window* window1 = new Window(140, 260, GUI::Width/2 - 140/2, GUI::Width/2 - 260/2);
    window1->titleString = "Calculator";

    Control* labelBox = new Control(130, 30, 5, 5);
    labelBox->backColor = 0xFFBBBBBB;
    window1->AddChild(labelBox);

    char* label = new char[2];
    label[0] = '0'; label[1] = '\0';
    textLabel = new Label(label);
    labelBox->AddChild(textLabel);

    int i = 0;
    for(int y = 0; y < 4; y++)
        for(int x = 0; x < 3 && i < 10; x++) {
            char* str = new char[2];
            str[0] = Convert::IntToString(i++)[0];
            str[1] = 0;

            Button* but = new Button(str);
            but->width = but->height = 30;
            but->x = 5 + x*(150/3);
            but->y = 37 + y*(120/3);
            but->MouseClick += ButtonClickHandler;
            window1->AddChild(but);
        }

    Button* plusButton = new Button("+");
    plusButton->width = plusButton->height = 30;
    plusButton->x = 55;
    plusButton->y = 157;
    plusButton->MouseClick += ButtonClickHandler;
    window1->AddChild(plusButton);

    Button* minButton = new Button("-");
    minButton->width = minButton->height = 30;
    minButton->x = 105;
    minButton->y = 157;
    minButton->MouseClick += ButtonClickHandler;
    window1->AddChild(minButton);

    Button* calcButton = new Button("Calculate");
    calcButton->width = 80;
    calcButton->height = 30;
    calcButton->x = 5;
    calcButton->y = 190;
    calcButton->MouseClick += CalculateButtonHandler;
    window1->AddChild(calcButton);

    Button* newButton = new Button("New");
    newButton->width = 45;
    newButton->height = 30;
    newButton->x = 90;
    newButton->y = 190;
    newButton->MouseClick += NewClickHandler;
    window1->AddChild(newButton);

    while (GUI::HasItems()) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    return 0;
}

void ButtonClickHandler(void* sender, MouseButtonArgs arg)
{
    if(calculated) { //Reset text
        delete textLabel->text; //Free previous text buffer
        textLabel->text = "";
        calculated = false;
    }

    int newStrLen = strlen(textLabel->text) + 1;
    char* newStr = new char[newStrLen];
    memcpy(newStr, textLabel->text, newStrLen-1);
    newStr[newStrLen-1] = ((Button*)sender)->label[0];
    textLabel->text = newStr;
}

void CalculateButtonHandler(void* sender, MouseButtonArgs arg)
{
    char* str = textLabel->text;

    bool plus = str_Contains(str, '+');
    bool min = str_Contains(str, '-');

    if((!plus && !min) || (plus && min)) {
        textLabel->text = "";
        return;
    }

    int index = str_IndexOf(str, plus ? '+' : '-');
    str[index] = '\0';

    char* part1 = str;
    char* part2 = str + index + 1;

    Print("Par1: %s Part2: %s\n", part1, part2);

    int int1 = Convert::StringToInt(part1);
    int int2 = Convert::StringToInt(part2);

    Print("Int1 %d %s Int2 %d\n", int1, plus ? "+" : "-", int2);
    
    int result = plus ? (int1 + int2) : (int1 - int2);
    
    char* resultStr = Convert::IntToString(result);
    int resultLen = strlen(resultStr);
    char* realStr = new char[resultLen + 1];
    memcpy(realStr, resultStr, resultLen);
    realStr[resultLen] = '\0';

    textLabel->text = realStr;
    calculated = true;
}