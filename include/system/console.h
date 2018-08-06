#ifndef __CACTUSOS__SYSTEM__CONSOLE_H
#define __CACTUSOS__SYSTEM__CONSOLE_H

#include <common/types.h>
#include <system/drivers/keyboard.h>
#include <common/memoryoperations.h>

namespace CactusOS
{
    namespace system
    {
        class Console
        {
        private:
            static KeyboardDriver* kb;
            static int XOffset;
            static int YOffset;

            static common::uint8_t fg;
            static common::uint8_t bg;

            static void Scroll();
        public:
            static bool CheckForEmptyString;
            static bool Started;
            static void SetKeyboard(KeyboardDriver* kb);
            static void SetColors(common::uint8_t fg, common::uint8_t bg);
            
            static void Write(char* str);
            static void WriteLine(char* str);
            static void WriteLine();
            static char* ReadLine();
            static void Clear();
        };
    }
}

#endif