#ifndef __CACTUSOS__INSTALLER__TEXTGUI_H
#define __CACTUSOS__INSTALLER__TEXTGUI_H

#include <system/bootconsole.h>

namespace CactusOS
{
    #define TEXT_COLOR system::VGA_COLOR_WHITE

    //Draw a simple gui using the VGA text mode
    class TextGUI
    {
    public:
        static void DisableCursor();
        static void SetPixel(int x, int y, char color, common::uint16_t character = ' ', char background = system::VGA_COLOR_BLUE);
        static void ClearScreen(char color = system::VGA_COLOR_BLACK);
        static void StatusBar(char* text, int percentage = 0);
        static void DrawString(char* text, int x, int y, char color = TEXT_COLOR, char background = system::VGA_COLOR_BLUE);
    };
}

#endif