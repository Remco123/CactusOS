#ifndef PROGRESS_H
#define PROGRESS_H

#include <types.h>
#include <gui/directgui.h>

using namespace LIBCactusOS;

class ProgressBar
{
private:
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    uint32_t background = 0xFFFFFFFF;
    uint32_t foreground = 0xFF00FF00;
    uint32_t borderColor = 0xFF000000;
public:
    ProgressBar(int x_p, int y_p, int width, int height);

    void SetValue(int v);
};

ProgressBar::ProgressBar(int x_p, int y_p, int width, int height)
{
    this->x = x_p;
    this->y = y_p;
    this->w = width;
    this->h = height;
}

void ProgressBar::SetValue(int v)
{
    DirectGUI::DrawFillRect(background, x, y, w + 1, h);
    DirectGUI::DrawRect(borderColor, x, y, w, h);
    if(v == 0)
        return;

    double fraction = (v/100.0);
    DirectGUI::DrawFillRect(foreground, x + 1, y + 1, (fraction * (double)w), h - 1);
}

#endif