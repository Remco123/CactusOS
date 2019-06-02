#include <gui/rect.h>

using namespace LIBCactusOS;

Rectangle::Rectangle(int w, int h, int x_p, int y_p)
{
    this->width = w;
    this->height = h;
    this->x = x_p;
    this->y = y_p;
}