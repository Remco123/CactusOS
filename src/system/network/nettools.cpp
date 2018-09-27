#include <system/network/nettools.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;

void printf(char*);
void printfHex(uint8_t);

void NetTools::PrintMac(uint48_t key)
{
    printfHex( key & 0xFF);
    printf(":");
    printfHex((key >> 8) & 0xFF);
    printf(":");
    printfHex((key >> 16) & 0xFF);
    printf(":");
    printfHex((key >> 24) & 0xFF);
    printf(":");
    printfHex((key >> 32) & 0xFF);
    printf(":");
    printfHex((key >> 40) & 0xFF);
}
void NetTools::PrintIP(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    //printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
    printf(Convert::IntToString(bytes[3])); printf(".");
    printf(Convert::IntToString(bytes[2])); printf(".");
    printf(Convert::IntToString(bytes[1])); printf(".");
    printf(Convert::IntToString(bytes[0])); 
}

uint32_t NetTools::MakeIP(common::uint8_t ip1, common::uint8_t ip2, common::uint8_t ip3, common::uint8_t ip4)
{
    return ((uint32_t)ip1 << 24)
        |  ((uint32_t)ip2 << 16)
        |  ((uint32_t)ip3 << 8)
        | (uint32_t)ip4;
}
uint32_t NetTools::ParseIP(char* str)
{
    char* parts[4];
    int partcount = 0;
    parts[partcount++] = str;

    while(*str)
    {
        if(*str == '.')
        {
            *str = 0;
            parts[partcount++] = str + 1;
        }
        str++;
    }

    uint8_t ip1 = Convert::StringToInt(parts[0]), ip2 = Convert::StringToInt(parts[1]), ip3 = Convert::StringToInt(parts[2]), ip4 = Convert::StringToInt(parts[3]);
    return ((uint32_t)ip1 << 24)
        |  ((uint32_t)ip2 << 16)
        |  ((uint32_t)ip3 << 8)
        |  (uint32_t)ip4;
}

void NetTools::PrintPacket(uint8_t* data, uint32_t size)
{
    printf("--------------# Packet #--------------\n");

    int x = 0;
    for(int i = 0; i < size; i++)
    {
        if(x == 16)
        {
            x = 0;
        }
        printfHex(data[i]);
        if(i < size)
            printf(" ");
        x++;
    }

    printf("\n-----------# End Of Packet #----------\n");
}