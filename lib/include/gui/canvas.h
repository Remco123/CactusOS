#ifndef __LIBCACTUSOS__GUI__CANVAS_H
#define __LIBCACTUSOS__GUI__CANVAS_H

#include <types.h>
#include <gui/fonts/font.h>

namespace LIBCactusOS
{
    class Canvas
    {
    public:
        void* bufferPointer;
        int Width;
        int Height;

        Canvas(void* buffer, int w, int h);

        void SetPixel(int x, int y, uint32_t color);
        uint32_t GetPixel(int x, int y);

        void Clear();
        void Clear(uint32_t color);
        void DrawHorizontalLine(uint32_t color, int dx, int x1, int y1);
        void DrawVerticalLine(uint32_t color, int dx, int x1, int y1);
        void DrawLine(uint32_t color, int x1, int y1, int x2, int y2);
        void DrawDiagonalLine(uint32_t color, int dx, int dy, int x1, int y1);
        void DrawRect(uint32_t color, int x, int y, int width, int height);
        void DrawFillRect(uint32_t color, int x_start, int y_start, int width, int height);
        void DrawCircle(uint32_t color, int x_center, int y_center, int radius);
        void DrawFillCircle(uint32_t color, int x_center, int y_center, int radius);
        void DrawEllipse(uint32_t color, int x_center, int y_center, int x_radius, int y_radius);

        void DrawString(Font* font, char* string, int x, int y, uint32_t color);
    };
}

#endif