#ifndef __CACTUSOS__SYSTEM__NETWORK__DNS_H
#define __CACTUSOS__SYSTEM__NETWORK__DNS_H

#include <common/types.h>
#include <system/network/networkmanager.h>
#include <system/network/udp.h>
#include <core/pit.h>
#include <common/string.h>

namespace CactusOS
{
    namespace system
    {
        class UDPSocket;

        class DNS
        {
            //DNS header structure
            struct DNS_HEADER
            {
                unsigned short id; // identification number
            
                unsigned char rd :1; // recursion desired
                unsigned char tc :1; // truncated message
                unsigned char aa :1; // authoritive answer
                unsigned char opcode :4; // purpose of message
                unsigned char qr :1; // query/response flag
            
                unsigned char rcode :4; // response code
                unsigned char cd :1; // checking disabled
                unsigned char ad :1; // authenticated data
                unsigned char z :1; // its z! reserved
                unsigned char ra :1; // recursion available
            
                unsigned short q_count; // number of question entries
                unsigned short ans_count; // number of answer entries
                unsigned short auth_count; // number of authority entries
                unsigned short add_count; // number of resource entries
            } __attribute__((packed));
            
            //Constant sized fields of query structure
            struct QUESTION
            {
                unsigned short qtype;
                unsigned short qclass;
            } __attribute__((packed));
            
            //Constant sized fields of the resource record structure
            struct R_DATA
            {
                unsigned short type;
                unsigned short _class;
                unsigned int ttl;
                unsigned short data_len;
            } __attribute__((packed));
            
            //Pointers to resource record contents
            struct RES_RECORD
            {
                unsigned char *name;
                struct R_DATA *resource;
                unsigned char *rdata;
            } __attribute__((packed));
            
            //Structure of a Query
            typedef struct
            {
                unsigned char *name;
                struct QUESTION *ques;
            } __attribute__((packed)) QUERY;

        private:
            NetworkManager* backend;
            core::PIT* pit;

            static DNS* instance;
            static void static_handle_udp(unsigned char* data, unsigned int size);
            void HandleUDP(unsigned char* data, unsigned int size);

            common::uint16_t RequestId;
        public:
            UDPSocket* dnsSocket;
            DNS(NetworkManager* backend, core::PIT* pit);

            void GetHostByName(const char* host, common::uint8_t* into);
            void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host);
        };
    }
}

#endif