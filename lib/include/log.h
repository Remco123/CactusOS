#ifndef __CACTUSOSLIB__LOG_H
#define __CACTUSOSLIB__LOG_H

namespace LIBCactusOS
{
    enum LogLevel
    {
        Info,
        Warning,
        Error
    };

    void Log(LogLevel level, char* msg);
}

#endif