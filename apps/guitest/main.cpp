#include <new.h>
#include <gui/guicom.h>
#include <log.h>
#include <time.h>
#include <string.h>

using namespace LIBCactusOS;

int main()
{
    Print("GUITest: Requesting context\n");
    GUICommunication::RequestContext(0xAB000000, 200, 200, 100, 100);

    char c = 0x0;
    while(1)
    {
        c += 100;
        memset((void*)0xAB000000, c, 200 * 200 * 4);
        Time::Sleep(1);
    }
    return 0;
}