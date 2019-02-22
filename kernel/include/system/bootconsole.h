#ifndef __CACTUSOS__SYSTEM__BOOTCONSOLE_H
#define __CACTUSOS__SYSTEM__BOOTCONSOLE_H

#include <common/types.h>
#include <system/serialport.h>

#include <stdarg.h>

namespace CactusOS
{
    namespace system
    {
        #define VGA_WIDTH 80
        #define VGA_HEIGHT 25

        enum vga_color {
            VGA_COLOR_BLACK = 0,
            VGA_COLOR_BLUE = 1,
            VGA_COLOR_GREEN = 2,
            VGA_COLOR_CYAN = 3,
            VGA_COLOR_RED = 4,
            VGA_COLOR_MAGENTA = 5,
            VGA_COLOR_BROWN = 6,
            VGA_COLOR_LIGHT_GREY = 7,
            VGA_COLOR_DARK_GREY = 8,
            VGA_COLOR_LIGHT_BLUE = 9,
            VGA_COLOR_LIGHT_GREEN = 10,
            VGA_COLOR_LIGHT_CYAN = 11,
            VGA_COLOR_LIGHT_RED = 12,
            VGA_COLOR_LIGHT_MAGENTA = 13,
            VGA_COLOR_LIGHT_BROWN = 14,
            VGA_COLOR_WHITE = 15
        };

        class BootConsole
        {
        private:
            static int XOffset;
            static int YOffset;
            static bool writeToSerial;

            static void Scroll();
        public:
            static common::uint8_t ForegroundColor;
            static common::uint8_t BackgroundColor;

            static void Init(bool enableSerial = false);

            static void Write(char c);
            static void Write(char* str);
            static void WriteLine(char* str);
            static void WriteLine();

            static void Clear();
            static void SetX(int x);
            static void SetY(int y);

            static common::uint16_t* GetBuffer();
        };
    }
}

#endif