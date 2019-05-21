#include <gui/directgui.h>
#include <gui/canvas.h>
#include <log.h>
#include <syscall.h>
#include <string.h>
#include <math.h>

using namespace LIBCactusOS;

Canvas* base;

bool DirectGUI::RequestFramebuffer()
{
    Log(Info, "This process is requesting a direct framebuffer");

    bool ret = DoSyscall(SYSCALL_GUI_GETLFB, DIRECT_GUI_ADDR);
    if(ret)
        base = new Canvas((void*)DIRECT_GUI_ADDR, WIDTH, HEIGHT);
    
    return ret;
}

void DirectGUI::SetPixel(uint32_t x, uint32_t y, uint32_t color)
{
    base->SetPixel(x, y, color);
}
uint32_t DirectGUI::GetPixel(uint32_t x, uint32_t y)
{
    return base->GetPixel(x, y);
}

void DirectGUI::Clear()
{
    base->Clear();
}
void DirectGUI::Clear(uint32_t color)
{
    base->Clear(color);
}
void DirectGUI::DrawHorizontalLine(uint32_t color, int dx, int x1, int y1)
{
    base->DrawHorizontalLine(color, dx, x1, y1);
}
void DirectGUI::DrawVerticalLine(uint32_t color, int dx, int x1, int y1)
{
    base->DrawVerticalLine(color, dx, x1, y1);
}
void DirectGUI::DrawDiagonalLine(uint32_t color, int dx, int dy, int x1, int y1)
{
    base->DrawDiagonalLine(color, dx, dy, x1, y1);
}
void DirectGUI::DrawLine(uint32_t color, int x1, int y1, int x2, int y2)
{
    base->DrawLine(color, x1, y1, x2, y2);
}
void DirectGUI::DrawRect(uint32_t color, int x, int y, int width, int height)
{
    base->DrawRect(color, x, y, width, height);
}
void DirectGUI::DrawFillRect(uint32_t color, int x_start, int y_start, int width, int height)
{
    base->DrawFillRect(color, x_start, y_start, width, height);
}
void DirectGUI::DrawCircle(uint32_t color, int x_center, int y_center, int radius)
{
    base->DrawCircle(color, x_center, y_center, radius);
}
void DirectGUI::DrawFillCircle(uint32_t color, int x_center, int y_center, int radius)
{
    base->DrawFillCircle(color, x_center, y_center, radius);
}
void DirectGUI::DrawEllipse(uint32_t color, int x_center, int y_center, int x_radius, int y_radius)
{
    base->DrawEllipse(color, x_center, y_center, x_radius, y_radius);
}