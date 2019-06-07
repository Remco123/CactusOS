#include <new.h>
#include <gui/gui.h>
#include <gui/widgets/button.h>
#include <gui/widgets/control.h>
#include <gui/widgets/window.h>
#include <gui/widgets/label.h>
#include <convert.h>
#include <string.h>
#include <log.h>
#include <proc.h>

using namespace LIBCactusOS;

void ButtonClickHandler(Button* sender, uint8_t button);
void CalculateButtonHandler(Button* sender, uint8_t button);

Label* textLabel = 0;
bool calculated = true;
int main()
{
    Print("Starting Calculator\n");
    GUI::Initialize();

    Context* screen1 = GUI::RequestContext(140, 260, 20 + 50*Process::ID, 25 + 50*Process::ID);
    if(screen1 == 0)
        return -1;

    Window* window1 = new Window(screen1, 140, 260);
    window1->titleString = "Calculator";

    Control* labelBox = new Control(130, 30, 5, 5);
    labelBox->backColor = 0xFFDDDDDD;
    window1->childs.push_back(labelBox);

    textLabel = new Label("0");
    labelBox->childs.push_back(textLabel);

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
            but->mouseClickHandler = GUI_MouseCall(ButtonClickHandler);
            window1->childs.push_back(but);
        }

    Button* plusButton = new Button("+");
    plusButton->width = plusButton->height = 30;
    plusButton->x = 55;
    plusButton->y = 157;
    plusButton->mouseClickHandler = GUI_MouseCall(ButtonClickHandler);
    window1->childs.push_back(plusButton);

    Button* minButton = new Button("-");
    minButton->width = minButton->height = 30;
    minButton->x = 105;
    minButton->y = 157;
    minButton->mouseClickHandler = GUI_MouseCall(ButtonClickHandler);
    window1->childs.push_back(minButton);

    Button* calcButton = new Button("Calculate");
    calcButton->width = 130;
    calcButton->height = 30;
    calcButton->x = 5;
    calcButton->y = 190;
    calcButton->mouseClickHandler = GUI_MouseCall(CalculateButtonHandler);
    window1->childs.push_back(calcButton);

    while(1) {
        GUI::DrawGUI();
        GUI::ProcessEvents();
    }

    return 0;
}

void ButtonClickHandler(Button* sender, uint8_t button)
{
    if(calculated) { //Reset text
        textLabel->text = "";
        calculated = false;
    }

    int newStrLen = strlen(textLabel->text) + 1;
    char* newStr = new char[newStrLen];
    memcpy(newStr, textLabel->text, newStrLen-1);

    newStr[newStrLen-1] = sender->label[0];
    textLabel->text = newStr;
}

void CalculateButtonHandler(Button* sender, uint8_t button)
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
    textLabel->text = Convert::IntToString(result);
    calculated = true;
}