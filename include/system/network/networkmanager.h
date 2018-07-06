#ifndef __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H
#define __CACTUSOS__SYSTEM__NETWORK__NETWORKMANAGER_H

#include <common/types.h>
#include <system/drivers/networkdriver.h>
#include <system/network/ethernet.h>

namespace CactusOS
{
    namespace system
    {
        class NetworkDriver;

        class NetworkManager
        {
        protected:
            NetworkDriver* netDevice;
            static NetworkManager* instance;
        public:
            NetworkManager(NetworkDriver* device);
            ~NetworkManager();

            void HandlePacket(common::uint8_t* packet, common::uint32_t size);
            void SendPacket(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE, common::uint8_t* buffer, common::uint32_t size);
        };
    }
}

#endif