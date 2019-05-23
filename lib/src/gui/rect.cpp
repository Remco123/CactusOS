#include <gui/rect.h>

using namespace LIBCactusOS;

Rectangle::Rectangle(uint32_t w, uint32_t h, uint32_t x_p, uint32_t y_p)
{
    this->width = w;
    this->height = h;
    this->x = x_p;
    this->y = y_p;
}