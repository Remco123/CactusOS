#include "terminalcontrol.h"
#include <string.h>
#include <proc.h>

using namespace LIBCactusOS;

TerminalControl::TerminalControl(int w, int h)
: Control(w, h)
{
    this->textBuffer = new char[TERM_WIDTH * TERM_HEIGH];
    memset(this->textBuffer, '\0', TERM_WIDTH * TERM_HEIGH);

    this->x = 0;
    this->y = 0;
}
TerminalControl::~TerminalControl()
{
    delete this->textBuffer;
}

void TerminalControl::Write(char c)
{
    if(c == 0)
        return;

    switch(c)
    {
        case '\n':
            {
                this->x = 0;
                if(this->y == TERM_HEIGH-1)
                    ScrollLine();
                else
                    this->y++;
            }
            break;
        case '\t':
            {
                for(int i = 0; i < 4; i++)
                    Write(' ');
            }
            break;
        case '\b':
            {
                if(this->x >= 1)
                {
                    this->x--;
                    textBuffer[this->y * TERM_WIDTH + this->x] = '\0';
                }
                else if(this->y >= 1) //We need to go one line up
                {
                    this->y--;
                    this->x = TERM_WIDTH-1;
                    textBuffer[this->y * TERM_WIDTH + this->x] = '\0';
                }
            }
            break;
        default:
            {
                if(this->x == TERM_WIDTH)
                {
                    this->x = 0;
                    if(this->y == TERM_HEIGH-1)
                        ScrollLine();
                    else
                        this->y++;
                }

                textBuffer[this->y * TERM_WIDTH + this->x] = c;
                this->x++;
            }
            break;
    }
}
void TerminalControl::ScrollLine()
{
    for(int i = 0; i < TERM_HEIGH; i++)
        memcpy(textBuffer + i*TERM_WIDTH, textBuffer + ((i+1)*TERM_WIDTH), TERM_WIDTH);

    for(int i = 0; i < TERM_WIDTH; i++)
        textBuffer[(TERM_HEIGH-1)*TERM_WIDTH + i] = '\0';
}
void TerminalControl::Clear()
{
    this->x = 0;
    this->y = 0;
    memset(this->textBuffer, '\0', TERM_WIDTH * TERM_HEIGH);
}
void TerminalControl::Write(char* str)
{
    int i = 0;
    while(str[i])
        Write(str[i++]);
}

char* TerminalControl::ReadCommand(char* prompt)
{
    Write(prompt);

    char* result = new char[100];

    uint8_t numChars = 0;
    while(true)
    {
        while(this->lastInputKey == 0)
            Process::Yield();
        
        switch(this->lastInputKey)
        {
            case '\n':
                {
                    //Remove cursor
                    textBuffer[this->y * TERM_WIDTH + this->x] = '\0';

                    //Create new line
                    Write('\n');

                    //terminate string
                    result[numChars] = '\0';

                    //reset key
                    this->lastInputKey = 0;
                    return result;
                }

            case '\b':
                {
                    if(numChars > 0)
                    {
                        Write('\b');
                        
                        result[numChars] = '\0';
                        numChars--;
                    }
                }
                break;
            default: 
                {
                    if(numChars < 100) {
                        result[numChars] = lastInputKey;
                        numChars++; 

                        Write(lastInputKey);
                    }
                }
                break;
        }
        this->lastInputKey = 0;
    }
}

void TerminalControl::DrawTo(Canvas* context, int x_abs, int y_abs)
{
    context->DrawFillRect(0xFF428052, x_abs, y_abs+1, width+1, height-1);

    char tmpStr[2];
    tmpStr[1] = '\0';

    for(int xp = 0; xp < TERM_WIDTH; xp++)
        for(int yp = 0; yp < TERM_HEIGH; yp++)
        {
            tmpStr[0] = textBuffer[yp * TERM_WIDTH + xp];
            context->DrawString(tmpStr, x_abs + xp*8 + 2, y_abs + 1 + yp*14, textColor);
        }
}

void TerminalControl::OnKeyPress(char key)
{
    lastInputKey = key;

    Control::OnKeyPress(key);
}