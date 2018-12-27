#ifndef __CACTUSOS__CORE__REGISTERS_H
#define __CACTUSOS__CORE__REGISTERS_H

#include <common/types.h>

namespace CactusOS
{
    namespace core
    {
        struct CPUState
        {
            struct //sGS
            {
                common::uint16_t GS;
                common::uint16_t hGS;
            };
            struct //sFS
            {
                common::uint16_t FS;
                common::uint16_t hFS;
            };
            struct //sES
            {
                common::uint16_t ES;
                common::uint16_t hES;
            };
            struct //sDS
            {
                common::uint16_t DS;
                common::uint16_t hDS;
            };
            union //sEDI
            {
                common::uint32_t EDI;
                common::uint16_t DI;
            };
            union //sESI
            {
                common::uint32_t ESI;
                common::uint16_t SI;
            };
            union //sEBP
            {
                common::uint32_t EBP;
                common::uint16_t BP;
            };
            union //sESP
            {
                common::uint32_t ESP;
                common::uint16_t SP;
            };
            union //sEBX
            {
                common::uint32_t EBX;
                common::uint16_t BX;
                struct
                {
                    common::uint8_t BL;
                    common::uint8_t BH;
                };
            };
            union //sEDX
            {
                common::uint32_t EDX;
                common::uint16_t DX;
                struct
                {
                    common::uint8_t DL;
                    common::uint8_t DH;
                };
            };
            union //sECX
            {
                common::uint32_t ECX;
                common::uint16_t CX;
                struct
                {
                    common::uint8_t CL;
                    common::uint8_t CH;
                };
            };
            union //sEAX
            {
                common::uint32_t EAX;
                common::uint16_t AX;
                struct
                {
                    common::uint8_t AL;
                    common::uint8_t AH;
                };
            };
            common::uint32_t InterruptNumber;
            common::uint32_t ErrorCode;

            union //sEIP
            {
                common::uint32_t EIP;
                common::uint16_t IP;
            };
            struct //sCS
            {
                common::uint16_t CS;
                common::uint16_t hCS;
            };
            union //sEFLAGS
            {
                common::uint32_t EFLAGS;
                common::uint16_t FLAGS;
            };
            union //sUserESP
            {
                common::uint32_t UserESP;
                common::uint16_t UserSP;
            };
            union //sUserSS
            {
                common::uint16_t UserSS;
                common::uint16_t hUserSS;
            };
        } __attribute__((packed));
    }
}

#endif