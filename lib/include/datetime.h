#ifndef __LIBCACTUSOS__DATETIME_H
#define __LIBCACTUSOS__DATETIME_H

namespace LIBCactusOS
{
    class DateTime
    {
    public:
        signed char Day = -1;
        signed char Month = -1;
        int Year = -1;

        signed char Seconds = -1;
        signed char Minutes = -1;
        signed char Hours = -1;

        /**
         * Get the current date and time of this system
        */
        static DateTime Current();

        /**
         * Convert DateTime into a readable string
        */
        char* ToString();
    };
}

#endif