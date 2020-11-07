#ifndef __CACTUSOS__SYSTEM__INPUT__KEYBOARDMANAGER_H
#define __CACTUSOS__SYSTEM__INPUT__KEYBOARDMANAGER_H

#include <system/input/keyboard.h>
#include <common/list.h>
#include <system/memory/fifostream.h>

namespace CactusOS
{
    namespace system
    {
        // Class responsable for managing all keyboard devices present on the system
        // Also provides a stream interface to read keystrokes 
        class KeyboardManager : public FIFOStream
        {
        public:
            // List of all keyboards present on system
            List<Keyboard*> keyboards;

            // Status of all keyboards
            KeyboardStatus sharedStatus;
        private:
            uint8_t ConvertToPS2(uint32_t key);
        public:
            KeyboardManager();

            // Handle a keypress or a change in modifier keys
            void HandleKeyChange(Keyboard* src, uint32_t key, bool pressed);
        };
    }
}

#endif