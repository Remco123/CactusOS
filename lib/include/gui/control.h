#ifndef __LIBCACTUSOS__GUI__CONTROL_H
#define __LIBCACTUSOS__GUI__CONTROL_H

#include <types.h>
#include <gui/rect.h>
#include <list.h>
#include <gui/canvas.h>

namespace LIBCactusOS
{
    class Control : public Rectangle
    {
    public:
        Control* parent;
        List<Control*> childs;

        uint32_t backColor = 0xFFCACDD1;
        uint32_t borderColor = 0xFF333333;
    
        Control(uint32_t w, uint32_t h);
        Control(uint32_t w, uint32_t h, uint32_t x, uint32_t y);
        ~Control();

        virtual void DrawTo(Canvas* context, uint32_t x_abs, uint32_t y_abs);
    };
}

#endif