#ifndef __CACTUSOS__SYSTEM__DRIVERS__RTL8139_H
#define __CACTUSOS__SYSTEM__DRIVERS__RTL8139_H

#include <common/types.h>
#include <common/memoryoperations.h>
#include <common/convert.h>

#include <core/port.h>
#include <core/pci.h>
#include <core/pci.h>

#include <system/drivers/networkdriver.h>

namespace CactusOS
{
    namespace system
    {
        class RTL8139 : public NetworkDriver
        {

            #define RX_BUF_SIZE 8192

            #define CAPR 0x38
            #define RX_READ_POINTER_MASK (~3)
            #define ROK                 (1<<0)
            #define RER                 (1<<1)
            #define TOK     (1<<2)
            #define TER     (1<<3)
            #define TX_TOK  (1<<15)

        private:
            char* rx_buffer;
            int tx_cur;
        public:
            RTL8139(core::PeripheralComponentInterconnectDeviceDescriptor *dev, core::InterruptManager* interrupts, core::PeripheralComponentInterconnectController* pci);
            ~RTL8139();
            
            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void HandleReceive();
            void SendData(common::uint8_t* data, common::uint32_t datalen);
            void Activate();
            void Reset();
        };
    }
}

#endif