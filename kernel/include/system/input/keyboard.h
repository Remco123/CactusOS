#ifndef __CACTUSOS__SYSTEM__INPUT__KEYBOARD_H
#define __CACTUSOS__SYSTEM__INPUT__KEYBOARD_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        // Structure for modifier keys that are keyboard specific
        struct InternalKeyboardStatus
        {
            bool LeftShift;
            bool RightShift;

            bool Alt;
            bool AltGr;
            
            bool LeftControl;
            bool RightControl;
        };

        // Structure for shared modifier keys between keyboards
        struct KeyboardStatus
        {
            bool CapsLock;
            bool NumLock;
        };
        
        // Types of keyboards currently supported
        enum KeyboardType
        {
            PS2,
            USB
        };

        // Interface for providing a common access for all keyboard devices
        class Keyboard
        {
        public:
            KeyboardType type;
            InternalKeyboardStatus status;
        public:
            Keyboard(KeyboardType type);

            // Update LED's on a keyboard device
            virtual void UpdateLEDS();
            
            // Checks if the buffer contains the given key, also returns position of key
            bool ContainsKey(common::uint8_t key, common::uint8_t* packet, int* pos);
        };
    }
}

#endif