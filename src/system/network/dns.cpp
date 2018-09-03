#include <system/network/dns.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::core;
using namespace CactusOS::system;

//Most of the code from: https://gist.github.com/fffaraz/9d9170b57791c28ccda9255b48315168

void printf(char*);

DNS* DNS::instance = 0;

uint8_t pseudo_rand_8() {

    static uint16_t seed = 0;
    return (uint8_t)(seed = (12657 * seed + 12345) % 256);
}

uint8_t ReceivedDNSIP[4];
bool ReceivedDNSAnswer = false;

DNS::DNS(NetworkManager* backend, PIT* pit)
{
    this->backend = backend;
    this->pit = pit;
    
    if(this->backend->dhcp->Dns == 0)
    {
        printf("There is no nameserver availible for use, disabling dns\n");
        return;
    }
    this->dnsSocket = backend->udp->Connect(backend->dhcp->Dns, 53);
    this->dnsSocket->receiveHandle = DNS::static_handle_udp;

    printf("DNS Availible\n");
}
void DNS::static_handle_udp(unsigned char* data, unsigned int size)
{
    DNS::instance->HandleUDP(data, size);
}
void DNS::HandleUDP(unsigned char* data, unsigned int size)
{
    printf("Received DNS answer\n");
    ReceivedDNSAnswer = true;
}
void DNS::GetHostByName(const char* host, common::uint8_t* into)
{
    unsigned char buf[512],*qname;
 
    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;
 
    //Set the DNS structure to standard queries
    dns = (struct DNS_HEADER *)&buf;
 
    dns->id = (unsigned short) Convert::ByteSwap ((uint16_t)pseudo_rand_8());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = Convert::ByteSwap ((uint16_t)1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;
 
    //point to the query portion
    qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];
 
    ChangetoDnsNameFormat(qname , (unsigned char*)host);
    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (String::strlen((const char*)qname) + 1)]; //fill it 
 
    qinfo->qtype = Convert::ByteSwap ((uint16_t) 0x0001 ); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = Convert::ByteSwap ((uint16_t)1); //its internet (lol)

    ReceivedDNSAnswer = false;
    printf("Sending DNS Packet...\n");
    this->dnsSocket->Send(buf, sizeof(struct DNS_HEADER) + (String::strlen((const char*)qname)+1) + sizeof(struct QUESTION));

    while(!ReceivedDNSAnswer);
    {
        printf("->");
        if(!ReceivedDNSAnswer) //tiny timeout TODO: Add a maximum of tries
            pit->Sleep(100);
    }

    MemoryOperations::memcpy(into, ReceivedDNSIP, 4);

    ReceivedDNSAnswer = false;
    printf("DNS Received (test)\n");
}

void DNS::ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host)
{
    int lock = 0 , i;
    String::strcat((char*)host,".");
     
    for(i = 0 ; i < String::strlen((char*)host) ; i++) 
    {
        if(host[i]=='.') 
        {
            *dns++ = i-lock;
            for(;lock<i;lock++) 
            {
                *dns++=host[lock];
            }
            lock++;
        }
    }
    *dns++='\0';
}