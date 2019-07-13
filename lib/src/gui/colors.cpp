#include <gui/colors.h>

using namespace LIBCactusOS;

uint32_t Colors::AlphaBlend(uint32_t color1, uint32_t color2)
{
    static const int AMASK = 0xFF000000;
    static const int RBMASK = 0x00FF00FF;
    static const int GMASK = 0x0000FF00;
    static const int AGMASK = AMASK | GMASK;
    static const int ONEALPHA = 0x01000000;

    unsigned int a = (color2 & AMASK) >> 24;
    unsigned int na = 255 - a;
    unsigned int rb = ((na * (color1 & RBMASK)) + (a * (color2 & RBMASK))) >> 8;
    unsigned int ag = (na * ((color1 & AGMASK) >> 8)) + (a * (ONEALPHA | ((color2 & GMASK) >> 8)));
    
    return ((rb & RBMASK) | (ag & AGMASK));
}