#ifndef __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86REGISTERS_H
#define __CACTUSOS__SYSTEM__VIRTUAL_8086__VM86REGISTERS_H

#include <common/types.h>

namespace CactusOS
{
    namespace system
    {
        struct VM86Registers
        {
            union
            {
                common::uint32_t EAX;
                common::uint16_t AX;
                struct
                {
                    common::uint8_t AL;
                    common::uint8_t AH;
                };
            };
            union
            {
                common::uint32_t EBX;
                common::uint16_t BX;
                struct
                {
                    common::uint8_t BL;
                    common::uint8_t BH;
                };
            };
            union
            {
                common::uint32_t ECX;
                common::uint16_t CX;
                struct
                {
                    common::uint8_t CL;
                    common::uint8_t CH;
                };
            };
            union
            {
                common::uint32_t EDX;
                common::uint16_t DX;
                struct
                {
                    common::uint8_t DL;
                    common::uint8_t DH;
                };
            };
            union
            {
                common::uint32_t ESI;
                common::uint16_t SI;
            };
            union
            {
                common::uint32_t EDI;
                common::uint16_t DI;
            };
            union
            {
                common::uint32_t EBP;
                common::uint16_t BP;
            };
            common::uint32_t DS;
            common::uint32_t ES;
            common::uint32_t FS;
            common::uint32_t GS;
        } __attribute__((packed));
    }
}

#endif