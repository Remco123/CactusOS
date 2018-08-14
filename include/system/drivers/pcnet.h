#ifndef __CACTUSOS__SYSTEM__DRIVERS__PCNET_H
#define __CACTUSOS__SYSTEM__DRIVERS__PCNET_H

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
        class PCNET : public NetworkDriver
        {
            struct InitializationBlock
            {
                common::uint16_t mode;
                unsigned reserved1 : 4;
                unsigned numSendBuffers : 4;
                unsigned reserved2 : 4;
                unsigned numRecvBuffers : 4;
                common::uint64_t physicalAddress : 48;
                common::uint16_t reserved3;
                common::uint64_t logicalAddress;
                common::uint32_t recvBufferDescrAddress;
                common::uint32_t sendBufferDescrAddress;
            } __attribute__((packed));
            
            
            struct BufferDescriptor
            {
                common::uint32_t address;
                common::uint32_t flags;
                common::uint32_t flags2;
                common::uint32_t avail;
            } __attribute__((packed));

            BufferDescriptor* sendBufferDescr;
            common::uint8_t sendBufferDescrMemory[2048+15];
            common::uint8_t sendBuffers[2*1024+15][8];
            common::uint8_t currentSendBuffer;
            
            BufferDescriptor* recvBufferDescr;
            common::uint8_t recvBufferDescrMemory[2048+15];
            common::uint8_t recvBuffers[2*1024+15][8];
            common::uint8_t currentRecvBuffer;
        public:
            PCNET(core::PeripheralComponentInterconnectDeviceDescriptor *dev, core::InterruptManager* interrupts, core::PeripheralComponentInterconnectController* pci);
            ~PCNET();
            
            common::uint32_t HandleInterrupt(common::uint32_t esp);
            void HandleReceive();
            void SendData(common::uint8_t* data, common::uint32_t datalen);
            void Activate();
            void Reset();
        };
    }
}

#endif