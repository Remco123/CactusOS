#ifndef __CACTUSOS__SYSTEM__COMPONENTS_H
#define __CACTUSOS__SYSTEM__COMPONENTS_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class SystemComponent
        {
        private:
            char* Name;
            char* Description;
        public:
            SystemComponent(char* name = 0, char* description = 0);

            char* GetComponentName();
            char* GetComponentDescription();
        };
    }
}

#endif