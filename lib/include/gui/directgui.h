#ifndef __CACTUSOSLIB__GUI__DIRECTGUI_H
#define __CACTUSOSLIB__GUI__DIRECTGUI_H

#include <types.h>
#include <gui/canvas.h>

namespace LIBCactusOS
{
    class DirectGUI
    {
    public:
        static bool RequestFramebuffer();
        static Canvas* GetCanvas();

        static void SetPixel(int x, int y, uint32_t color);
        static uint32_t GetPixel(int x, int y);

        static void Clear();
        static void Clear(uint32_t color);
        static void DrawHorizontalLine(uint32_t color, int dx, int x1, int y1);
        static void DrawVerticalLine(uint32_t color, int dx, int x1, int y1);
        static void DrawLine(uint32_t color, int x1, int y1, int x2, int y2);
        static void DrawDiagonalLine(uint32_t color, int dx, int dy, int x1, int y1);
        static void DrawRect(uint32_t color, int x, int y, int width, int height);
        static void DrawFillRect(uint32_t color, int x_start, int y_start, int width, int height);
        static void DrawCircle(uint32_t color, int x_center, int y_center, int radius);
        static void DrawFillCircle(uint32_t color, int x_center, int y_center, int radius);
        static void DrawEllipse(uint32_t color, int x_center, int y_center, int x_radius, int y_radius);

        static void DrawChar(char character, int x, int y, uint32_t color);
        static void DrawString(char* string, int x, int y, uint32_t color);
    };
}

#endif