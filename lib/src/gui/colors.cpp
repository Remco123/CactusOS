#include <gui/colors.h>

using namespace LIBCactusOS;

static const int AMASK = 0xFF000000;
static const int RBMASK = 0x00FF00FF;
static const int GMASK = 0x0000FF00;
static const int AGMASK = AMASK | GMASK;
static const int ONEALPHA = 0x01000000;

const uint32_t Colors::AlphaBlend(uint32_t color1, uint32_t color2)
{
    uint32_t a = (color2 & AMASK) >> 24;

    if(a == 0)
        return color1;
    else if(a == 255)
        return color2;
    else
    {
        uint32_t na = 255 - a;
        uint32_t rb = ((na * (color1 & RBMASK)) + (a * (color2 & RBMASK))) >> 8;
        uint32_t ag = (na * ((color1 & AGMASK) >> 8)) + (a * (ONEALPHA | ((color2 & GMASK) >> 8)));

        return ((rb & RBMASK) | (ag & AGMASK));
    }
}

const uint32_t Colors::FromARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}