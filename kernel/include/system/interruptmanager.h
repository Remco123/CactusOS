#ifndef __CACTUSOS__SYSTEM__INTERRUPTMANAGER_H
#define __CACTUSOS__SYSTEM__INTERRUPTMANAGER_H

#include <common/list.h>
#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        class InterruptHandler
        {
        public: 
            InterruptHandler(common::uint8_t intNumber);
            virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
        };


        class InterruptManager
        {
        private:
            static List<InterruptHandler*>* interruptCallbacks[256];
        public:
            static void Initialize();
            static common::uint32_t HandleInterrupt(common::uint8_t interrupt, common::uint32_t esp);

            static void AddHandler(InterruptHandler* handler, common::uint8_t interrupt);
            static void RemoveHandler(InterruptHandler* handler, common::uint8_t interrupt);
        };  
    }
}


#endif