#include <system/serialport.h>
#include <system/log.h>
#include <stdarg.h>
#include <common/convert.h>
#include <common/string.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

extern "C" void _putDebugChar(char a)
{
    Serialport::Write(a);
}
extern "C" int _getDebugChar()
{
    return Serialport::Read();
}

extern "C" void _fprintf(int stream, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    
    while (*format != '\0') { 
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			uint32_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			Print(format, amount);
			format += amount;
			continue;
		}
 
		const char* format_begun_at = format++;
 
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(args, int /* char promotes to int */);
			Print(&c, sizeof(c));
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(args, const char*);
			uint32_t len = String::strlen(str);
			Print(str, len);
         } else if(*format == 'd') {
            format++;
            int n = va_arg(args, int);
            int numChars = 0;
            if (n < 0) { n = -n; numChars++; Print("-", 1); }

            int temp = n;
            do
            {
                numChars++;
                temp /= 10;
            } while (temp);

            Print(Convert::IntToString(n), numChars);
        } else if(*format == 'b') {
            format++;
            uint8_t n = va_arg(args, int);
            char* str = Convert::IntToHexString(n);
            Print("0x", 2); Print(str, sizeof(uint8_t)<<1);
            delete str;
        } else if(*format == 'w') {
            format++;
            uint16_t n = va_arg(args, int);
            char* str = Convert::IntToHexString(n);
            Print("0x", 2); Print(str, sizeof(uint16_t)<<1);
            delete str;
        } else if(*format == 'x') {
            format++;
            uint32_t n = va_arg(args, int);
            char* str = Convert::IntToHexString(n);
            Print("0x", 2); Print(str, sizeof(uint32_t)<<1);
            delete str;
        } else {
			format = format_begun_at;
			uint32_t len = String::strlen(format);
			Print(format, len);
			format += len;
		}
	}

    va_end(args);
}