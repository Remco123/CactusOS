#ifndef __CACTUSOS__SYSTEM__DRIVER_H
#define __CACTUSOS__SYSTEM__DRIVER_H

namespace CactusOS
{
    namespace system
    {
        namespace drivers
        {
            class Driver
            {
            private:
                char* Name;
                char* Description;
            public:
                Driver(char* name = 0, char* description = 0);

                char* GetDriverName();
                char* GetDriverDescription();

                virtual bool Initialize();
            };
        }
    }
}

#endif