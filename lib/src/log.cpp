#include <log.h>
#include <syscall.h>
#include <stdarg.h>
#include <types.h>
#include <string.h>
#include <convert.h>
#include <proc.h>

using namespace LIBCactusOS;

void LIBCactusOS::Log(LogLevel level, char* msg)
{
    DoSyscall(SYSCALL_LOG, level, (int)msg);
}

void printLen(const char* data, uint32_t length)
{
    Process::WriteStdOut((char*)data, (int)length);
}

void LIBCactusOS::Print(const char* __restrict__ format, ...)
{
    va_list parameters;
	va_start(parameters, format);
 
	while (*format != '\0') { 
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			uint32_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			printLen(format, amount);
			format += amount;
			continue;
		}
 
		const char* format_begun_at = format++;
 
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			printLen(&c, sizeof(c));
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			uint32_t len = strlen(str);
			printLen(str, len);
         } else if(*format == 'd') {
            format++;
            int n = va_arg(parameters, int);
            int numChars = 0;
            if (n < 0) { n = -n; numChars++; printLen("-", 1); }

            int temp = n;
            do
            {
                numChars++;
                temp /= 10;
            } while (temp);

            printLen(Convert::IntToString(n), numChars);
        } else if(*format == 'b') {
            format++;
            uint8_t n = va_arg(parameters, int);
            char* str = Convert::IntToHexString(n);
            printLen("0x", 2); printLen(str, sizeof(uint8_t)<<1);
            delete str;
        } else if(*format == 'w') {
            format++;
            uint16_t n = va_arg(parameters, int);
            char* str = Convert::IntToHexString(n);
            printLen("0x", 2); printLen(str, sizeof(uint16_t)<<1);
            delete str;
        } else if(*format == 'x') {
            format++;
            uint32_t n = va_arg(parameters, int);
            char* str = Convert::IntToHexString(n);
            printLen("0x", 2); printLen(str, sizeof(uint32_t)<<1);
            delete str;
        } else if(*format == 'f') {
            format++;
            double n = va_arg(parameters, double);
            if(n < 0.0)
            {
                printLen("-", 1);
                n = -n;
            }
            if(n != n) {//Not a number
                printLen("NaN", 3);
                return;   
            }

            //Print integer part
            Print("%d", (int)n);
        
            // remove the integer part
            n -= (double)((int)n);

            if(n != 0.0)
                // now on to the decimal potion
                printLen(".", 1);

            /* on every iteration, make sure there are still decimal places left that are non-zero,
            and make sure we're still within the user-defined precision range. */
            int cur_prec = 1;
            while(n > (double)((int)n) && cur_prec++ < 8)
            {
                // move the next decimal into the integer portion and print it
                n *= 10;
                Print("%d", (int)n);
            
                /* if the nue is == the floored nue (integer portion),
                then there are no more decimal places that are non-zero. */
                if(n == (double)((int)n))
                    break;
            
                // subtract the integer portion
                n -= (double)((int)n);
            }

        } else {
			format = format_begun_at;
			uint32_t len = strlen(format);
			printLen(format, len);
			format += len;
		}
	}
 
	va_end(parameters);
}