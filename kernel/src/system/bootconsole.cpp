#include <system/bootconsole.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

/*/////////////////
// Static variable initialisations
/*/////////////////
int BootConsole::XOffset = 0;
int BootConsole::YOffset = 0;
uint8_t BootConsole::BackgroundColor = VGA_COLOR_BLACK; //Default console background color
uint8_t BootConsole::ForegroundColor = VGA_COLOR_WHITE; //Default console foreground color
bool BootConsole::writeToSerial = false;

/*/////////////////
// Variables
/*/////////////////
static uint16_t* videoMemory = (uint16_t*)0xC00B8000;


/*/////////////////
// Private functions
/*/////////////////
void BootConsole::Scroll()
{
    for(int i = 0; i < 24; i++){
        for (int m = 0; m < 80; m++){
            videoMemory[i * 80 + m] = videoMemory[(i + 1) * 80 + m];
        }
    }

    for(int x = 0; x < 80; x++)
    {
        uint16_t attrib = (BackgroundColor << 4) | (ForegroundColor & 0x0F);
        volatile uint16_t * where;
        where = (volatile uint16_t *)videoMemory + (24 * VGA_WIDTH + x) ;
        *where = ' ' | (attrib << 8);
    }
}



/*/////////////////
// Public functions
/*/////////////////
void BootConsole::Init(bool enableSerial)
{
    BootConsole::writeToSerial = enableSerial;
    if(enableSerial)
    {
        Serialport::Init(COMPort::COM1);
        BootConsole::WriteLine("Start of serial log for CactusOS");
    }
}

void BootConsole::Write(char c)
{
    static char* str = " ";
    str[0] = c;
    Write(str);
}

void BootConsole::Write(char* str)
{
    if (writeToSerial)
        Serialport::WriteStr(str);

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                XOffset = 0;
                YOffset++;
                break;
            case '\t':
                Write("    "); //4 spaces for tab
                break;
            default:
                uint16_t attrib = (BackgroundColor << 4) | (ForegroundColor & 0x0F);
                volatile uint16_t * where;
                where = (volatile uint16_t *)videoMemory + (YOffset * 80 + XOffset) ;
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
void BootConsole::WriteLine(char* str)
{
    BootConsole::Write(str);
    BootConsole::Write("\n");
}
void BootConsole::WriteLine()
{
    BootConsole::Write("\n");
}
void BootConsole::Clear()
{
    for(int y = 0; y < VGA_HEIGHT; y++)
        for(int x = 0; x < VGA_WIDTH; x++) {
                uint16_t attrib = (BackgroundColor << 4) | (ForegroundColor & 0x0F);
                volatile uint16_t * where;
                where = (volatile uint16_t *)videoMemory + (y * VGA_WIDTH + x) ;
                *where = ' ' | (attrib << 8);
        }

    XOffset = 0;
    YOffset = 0;
}
uint16_t* BootConsole::GetBuffer()
{
    return videoMemory;
}

void BootConsole::SetX(int x)
{
    XOffset = x;
}
void BootConsole::SetY(int y)
{
    YOffset = y;
}