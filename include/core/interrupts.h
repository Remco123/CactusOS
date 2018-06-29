#ifndef __CACTUSOS__CORE__INTERUPTS_H
#define __CACTUSOS__CORE__INTERUPTS_H

#include <core/gdt.h>
#include <core/port.h>
#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        class InterruptManager;

        class InterruptHandler
        {
        protected:
            common::uint8_t InterruptNumber;
            InterruptManager* interruptManager;
            InterruptHandler(InterruptManager* interruptManager, common::uint8_t InterruptNumber);
            ~InterruptHandler();
        public:
            virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
        };
        
        class InterruptManager
        {
            friend class InterruptHandler;
        protected:
            static InterruptManager* ActiveInterruptManager;
            InterruptHandler* handlers[256];

            struct GateDescriptor
            {
                common::uint16_t handlerAddressLowBits;
                common::uint16_t gdt_codeSegmentSelector;
                common::uint8_t reserved;
                common::uint8_t access;
                common::uint16_t handlerAddressHighBits;
            } __attribute__((packed));

            static GateDescriptor interruptDescriptorTable[256];

            struct InterruptDescriptorTablePointer
            {
                common::uint16_t size;
                common::uint32_t base;
            } __attribute__((packed));

            common::uint16_t hardwareInterruptOffset;
            static void SetInterruptDescriptorTableEntry(common::uint8_t interrupt,
                common::uint16_t codeSegmentSelectorOffset, void (*handler)(),
                common::uint8_t DescriptorPrivilegeLevel, common::uint8_t DescriptorType);


            static void InterruptIgnore();

            static void HandleInterruptRequest0x00();
            static void HandleInterruptRequest0x01();
            static void HandleInterruptRequest0x02();
            static void HandleInterruptRequest0x03();
            static void HandleInterruptRequest0x04();
            static void HandleInterruptRequest0x05();
            static void HandleInterruptRequest0x06();
            static void HandleInterruptRequest0x07();
            static void HandleInterruptRequest0x08();
            static void HandleInterruptRequest0x09();
            static void HandleInterruptRequest0x0A();
            static void HandleInterruptRequest0x0B();
            static void HandleInterruptRequest0x0C();
            static void HandleInterruptRequest0x0D();
            static void HandleInterruptRequest0x0E();
            static void HandleInterruptRequest0x0F();
            static void HandleInterruptRequest0x31();

            static void HandleException0x00();
            static void HandleException0x01();
            static void HandleException0x02();
            static void HandleException0x03();
            static void HandleException0x04();
            static void HandleException0x05();
            static void HandleException0x06();
            static void HandleException0x07();
            static void HandleException0x08();
            static void HandleException0x09();
            static void HandleException0x0A();
            static void HandleException0x0B();
            static void HandleException0x0C();
            static void HandleException0x0D();
            static void HandleException0x0E();
            static void HandleException0x0F();
            static void HandleException0x10();
            static void HandleException0x11();
            static void HandleException0x12();
            static void HandleException0x13();

            static common::uint32_t HandleInterrupt(common::uint8_t interrupt, common::uint32_t esp);
            common::uint32_t DoHandleInterrupt(common::uint8_t interrupt, common::uint32_t esp);
        public:
            InterruptManager(common::uint16_t hardwareInterruptOffset, GlobalDescriptorTable* globalDescriptorTable);
            ~InterruptManager();
            common::uint16_t HardwareInterruptOffset();
            void Activate();
            void Deactivate();
        };
    }
}

#endif