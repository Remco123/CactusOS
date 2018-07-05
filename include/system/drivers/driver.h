#ifndef __CACTUSOS__SYSTEM__DRIVERS__DRIVER_H
#define __CACTUSOS__SYSTEM__DRIVERS__DRIVER_H

#include <common/types.h>
#include <core/pci.h>

namespace CactusOS
{
    namespace system
    {
        enum DriverType
        {
            Network,
            Audio,
            Unkown
        };

        class Driver
        {
        friend class DriverManager;
        protected:
            DriverType Type;
        public:
            Driver();
            ~Driver();

            virtual void Activate();
        };
    }
}

#endif