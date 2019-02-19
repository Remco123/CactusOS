#include <system/log.h>

#include <system/bootconsole.h>
#include <system/serialport.h>
#include <system/system.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

char* logLevelMessage[3] {
    "[Info]",
    "[Warning]",
    "[Error]"
};

#if LOG_SHOW_MS
uint32_t GetMSSinceBoot()
{
    if(System::pit != 0)
        return System::pit->Ticks();
    return 0;
}
#endif

void CactusOS::system::Log(LogLevel level, char* msg)
{
    if(System::bootState == BootState::Booting)
    {
        #if LOG_SHOW_MS
        BootConsole::Write("["); BootConsole::Write(Convert::IntToString(GetMSSinceBoot())); BootConsole::Write("] ");
        #endif
        uint8_t prevColor = BootConsole::ForegroundColor;
        switch (level)
        {
            case Info:
                BootConsole::ForegroundColor = VGA_COLOR_BLACK;
                break;
            case Warning:
                BootConsole::ForegroundColor = VGA_COLOR_BROWN;
                break;
            case Error:
                BootConsole::ForegroundColor = VGA_COLOR_RED;
                break;
        }
        BootConsole::Write(logLevelMessage[level]); BootConsole::Write(": ");
        
        BootConsole::ForegroundColor = prevColor;
        BootConsole::WriteLine(msg);
    }
    else if(Serialport::Initialized)
    {
        #if LOG_SHOW_MS
        Serialport::WriteStr("["); Serialport::WriteStr(Convert::IntToString(GetMSSinceBoot())); Serialport::WriteStr("] ");
        #endif
        Serialport::WriteStr(logLevelMessage[level]); Serialport::WriteStr(": "); Serialport::WriteStr(msg); Serialport::WriteStr("\n");
    }
}