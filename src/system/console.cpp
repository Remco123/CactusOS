#include <system/console.h>

using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;
using namespace CactusOS;

KeyboardDriver* Console::kb = 0;
int Console::XOffset = 0;
int Console::YOffset = 0;
uint8_t Console::fg = 0xF; //Default white
uint8_t Console::bg = 0; //Default black
bool Console::CheckForEmptyString = false;
bool Console::Started = false;

static uint16_t* VideoMemory = (uint16_t*)0xb8000;

void Console::SetKeyboard(KeyboardDriver* kb)
{
    Console::kb = kb;
}
void Console::SetColors(uint8_t fg, uint8_t bg)
{
    Console::bg = bg;
    Console::fg = fg;
}

void Console::Scroll(){
    for(int i = 0; i < 25; i++){
        for (int m = 0; m < 80; m++){
            VideoMemory[i * 80 + m] = VideoMemory[(i + 1) * 80 + m];
        }
    }
}
            
void Console::Write(char* str)
{
    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                XOffset = 0;
                YOffset++;
                break;
            default:
                uint16_t attrib = (bg << 4) | (fg & 0x0F);
                volatile uint16_t * where;
                where = (volatile uint16_t *)0xB8000 + (YOffset * 80 + XOffset) ;
                *where = str[i] | (attrib << 8);
                XOffset++;
                break;
        }

        if(XOffset >= 80)
        {
            XOffset = 0;
            YOffset++;
        }

        if(YOffset >= 25)
        {
            Scroll();
            XOffset = 0;
            YOffset = 24;
        }
    }
}
void Console::WriteLine(char* str)
{
    Console::Write(str);
    Console::Write("\n");
}
void Console::WriteLine()
{
    Console::Write("\n");
}
char* Console::ReadLine()
{
    if(Console::kb == 0)
    {
        Console::WriteLine("Keyboard is null so we can not read a line from it\n");
        return;
    }

    static char buffer[100];
    MemoryOperations::memset(buffer, 0, 100); //clear previous buffer
    uint8_t numChars = 0;

    bool InputDone = false;
    while(!InputDone)
    {
        char input = Console::kb->GetKey(true);
        switch(input)
        {
            case '\n':
                if(CheckForEmptyString && numChars > 0)
                {
                    WriteLine();
                    return buffer;
                }
                else
                {
                    WriteLine();
                    return buffer;
                }
                break;
            case '\b':
                if(numChars > 0)
                {
                    buffer[numChars - 1] = '\0';
                    numChars--;
                    XOffset -= 1;
                    Console::Write(" "); //Clear old char
                    XOffset -= 1;
                }
                break;
            default:
                buffer[numChars] = input;
                numChars++;

                Console::Write(Convert::CharToString(input));
                break;
        }
    }
}

void Console::Clear()
{
    for(int i = 0; i < 25; i++){
        MemoryOperations::memset(VideoMemory + (i * 80), 0x0, 80 * 2);
    }

    XOffset = 0;
    YOffset = 0;
}