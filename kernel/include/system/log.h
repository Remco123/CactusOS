#ifndef __CACTUSOS__SYSTEM__LOG_H
#define __CACTUSOS__SYSTEM__LOG_H

namespace CactusOS
{
    namespace system
    {
        #define LOG_SHOW_MS 1

        enum LogLevel
        {
            Info,
            Warning,
            Error
        };

        void Log(LogLevel level, char* msg);
    }
}

#endif