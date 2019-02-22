#include <system/log.h>

#include <system/bootconsole.h>
#include <system/serialport.h>
#include <system/system.h>

#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>

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

static void print(const char* data, uint32_t length) {
    if(System::bootState == BootState::Booting)
    {
        for (uint32_t i = 0; i < length; i++)
            BootConsole::Write(data[i]);
    }
    else if(Serialport::Initialized)
    {
        for (uint32_t i = 0; i < length; i++)
            Serialport::Write(data[i]);
    }
}

void CactusOS::system::Log(LogLevel level, const char* __restrict__ format, ...)
{
    uint8_t prevColor = BootConsole::ForegroundColor;
    if(System::bootState == BootState::Booting)
    {
        #if LOG_SHOW_MS
        BootConsole::Write("["); BootConsole::Write(Convert::IntToString(GetMSSinceBoot())); BootConsole::Write("] ");
        #endif
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
    }
    else if(Serialport::Initialized)
    {
        #if LOG_SHOW_MS
        Serialport::WriteStr("["); Serialport::WriteStr(Convert::IntToString(GetMSSinceBoot())); Serialport::WriteStr("] ");
        #endif
        Serialport::WriteStr(logLevelMessage[level]); Serialport::WriteStr(": ");
    }

    va_list parameters;
	va_start(parameters, format);
 
	while (*format != '\0') { 
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			uint32_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			print(format, amount);
			format += amount;
			continue;
		}
 
		const char* format_begun_at = format++;
 
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			print(&c, sizeof(c));
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			uint32_t len = String::strlen(str);
			print(str, len);
         } else if(*format == 'd') {
            format++;
            int n = va_arg(parameters, int);
            int numChars = 0;
            if (n < 0) { n = -n; numChars++; print("-", 1); }

            int temp = n;
            do
            {
                numChars++;
                temp /= 10;
            } while (temp);

            print(Convert::IntToString(n), numChars);
        } else {
			format = format_begun_at;
			uint32_t len = String::strlen(format);
			print(format, len);
			format += len;
		}
	}
 
	va_end(parameters);

    print("\n", 1);

    if(System::bootState == Booting)
        BootConsole::ForegroundColor = prevColor;
}