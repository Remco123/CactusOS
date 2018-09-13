#ifndef __CACTUSOS__SYSTEM__DRIVERS__NETWORKDRIVER_H
#define __CACTUSOS__SYSTEM__DRIVERS__NETWORKDRIVER_H

#include <common/types.h>
#include <common/memoryoperations.h>
#include <system/drivers/driver.h>
//#include <system/network/networkmanager.h>

namespace CactusOS
{
    namespace system
    {
        //class NetworkManager; //Forward decleration

        class NetworkDriver : public Driver, public core::InterruptHandler
        {
        //friend class NetworkManager;
        protected:
            core::PeripheralComponentInterconnectDeviceDescriptor* device; //The device this driver is attached to
            //system::NetworkManager* NetManager;
            
            common::uint8_t MAC[6]; //Every NIC has a mac
        public:
            NetworkDriver(core::PeripheralComponentInterconnectDeviceDescriptor* dev, core::InterruptManager* interrupts);
            ~NetworkDriver();

            virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
            virtual void HandleReceive();
            virtual void SendData(common::uint8_t* data, common::uint32_t datalen);
            virtual void Activate();
            virtual void Reset();
            virtual common::uint64_t GetMacAddressBE();
        };
    }
}

#endif