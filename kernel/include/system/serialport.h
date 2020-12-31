#ifndef __CACTUSOS__SYSTEM__SERIALPORT_H
#define __CACTUSOS__SYSTEM__SERIALPORT_H

#include <common/types.h>
#include <core/port.h>
#include <system/serialport.h>

namespace CactusOS
{
    namespace system
    {
        enum COMPort
        {
            COM1 = 0x3F8,
            COM2 = 0x2F8,
            COM3 = 0x3E8,
            COM4 = 0x2E8
        };

        class Serialport
        {
        private:
            static COMPort PortAddress;
        public:
            static int SerialReceiveReady();
            static int SerialSendReady();

            static bool Initialized;
            static void Init(COMPort port);

            static char Read();
            static void Write(char a);
            static void WriteStr(char* str);
        };
    }
}

#endif