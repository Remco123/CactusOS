#include <gui/canvas.h>
#include <string.h>
#include <math.h>
#include <gui/colors.h>

using namespace LIBCactusOS;

Canvas::Canvas(void* buffer, int w, int h)
{
    this->Width = w;
    this->Height = h;
    this->bufferPointer = buffer;
}

void Canvas::SetPixel(int x, int y, uint32_t color)
{
    *(uint32_t*)((uint32_t)bufferPointer + (y * Width * 4 + x * 4)) = color;
}
uint32_t Canvas::GetPixel(int x, int y)
{
    return *(uint32_t*)((uint32_t)bufferPointer + (y * Width * 4 + x * 4));
}

void Canvas::Clear()
{
    memset((void*)bufferPointer, 0, Width*Height*4);
}
void Canvas::Clear(uint32_t color)
{
    uint32_t* buf = (uint32_t*)bufferPointer;       
    for(uint32_t index = 0; index < (uint32_t)(Width*Height); index++)
        buf[index] = color;
}
void Canvas::DrawHorizontalLine(uint32_t color, int dx, int x1, int y1)
{
    for (int i = 0; i < dx; i++)
        SetPixel(x1 + i, y1, color);
}
void Canvas::DrawVerticalLine(uint32_t color, int dy, int x1, int y1)
{
    for(int i = 0; i < dy; i++)
        SetPixel(x1, y1 + i, color);
}
void Canvas::DrawDiagonalLine(uint32_t color, int dx, int dy, int x1, int y1)
{
    int i, sdx, sdy, dxabs, dyabs, x, y, px, py;

    dxabs = Math::Abs(dx);
    dyabs = Math::Abs(dy);
    sdx = Math::Sign(dx);
    sdy = Math::Sign(dy);
    x = dyabs >> 1;
    y = dxabs >> 1;
    px = x1;
    py = y1;

    if (dxabs >= dyabs) // the line is more horizontal than vertical
    {
        for (i = 0; i < dxabs; i++)
        {
            y += dyabs;
            if (y >= dxabs)
            {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;
            SetPixel(px, py, color);
        }
    }
    else // the line is more vertical than horizontal
    {
        for (i = 0; i < dyabs; i++)
        {
            x += dxabs;
            if (x >= dyabs)
            {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;
            SetPixel(px, py, color);
        }
    }
}
void Canvas::DrawLine(uint32_t color, int x1, int y1, int x2, int y2)
{
    int dx, dy;

    dx = x2 - x1;      /* the horizontal distance of the line */
    dy = y2 - y1;      /* the vertical distance of the line */

    if (dy == 0) /* The line is horizontal */
    {
        if(dx < 0)
            DrawHorizontalLine(color, Math::Abs(dx), x2, y1);
        else
            DrawHorizontalLine(color, dx, x1, y1);

        return;
    }

    if (dx == 0) /* the line is vertical */
    {
        if(dy < 0)
            DrawVerticalLine(color, Math::Abs(dy), x1, y2);
        else
            DrawVerticalLine(color, dy, x1, y1);
        
        return;
    }

    /* the line is neither horizontal neither vertical, is diagonal then! */
    DrawDiagonalLine(color, dx, dy, x1, y1);
}
void Canvas::DrawRect(uint32_t color, int x, int y, int width, int height)
{
    int xa = x;
    int ya = y;

    /* The vertex B has the same y coordinate of A but x is moved of width pixels */
    int xb = x + width;
    int yb = y;

    /* The vertex C has the same x coordiate of A but this time is y that is moved of height pixels */
    int xc = x;
    int yc = y + height;

    /* The Vertex D has x moved of width pixels and y moved of height pixels */
    int xd = x + width;
    int yd = y + height;

    /* Draw a line betwen A and B */
    DrawLine(color, xa, ya, xb, yb);

    /* Draw a line between A and C */
    DrawLine(color, xa, ya, xc, yc);

    /* Draw a line between B and D */
    DrawLine(color, xb, yb, xd, yd + 1);

    /* Draw a line between C and D */
    DrawLine(color, xc, yc, xd, yd);
}

void Canvas::DrawRoundedRect(uint32_t color, int x, int y, int width, int height, int radius)
{
    // Draw the four lines
    DrawHorizontalLine(color, width - 2 * radius, x + radius, y);               // Top
    DrawHorizontalLine(color, width - 2 * radius, x + radius, y + height - 1);  // Bottom
    DrawVerticalLine(color, height - 2 * radius, x, y + radius);                // Left
    DrawVerticalLine(color, height - 2 * radius, x + width - 1, y + radius);    // Right
 
    // Draw the four corners
    DrawCircleHelper(x + radius, y + radius, radius, 1, color);
    DrawCircleHelper(x + width - radius - 1, y + radius, radius, 2, color);
    DrawCircleHelper(x + width - radius - 1, y + height - radius - 1, radius, 4, color);
    DrawCircleHelper(x + radius, y + height - radius - 1, radius, 8, color);
}

void Canvas::DrawFillRoundedRect(uint32_t color, int x, int y, int width, int height, int radius)
{
    // Draw the body
    DrawFillRect(color, x + radius, y, width - 2 * radius + 1, height);
 
    // Draw the four corners
    FillCircleHelper(x + width - radius - 1, y + radius, radius, 1, height - 2 * radius - 1, color);
    FillCircleHelper(x + radius, y + radius, radius, 2, height - 2 * radius - 1, color);
}

void Canvas::DrawFillRect(uint32_t color, int x_start, int y_start, int width, int height)
{
    for (int y = y_start; y < y_start + height; y++)
    {
        DrawLine(color, x_start, y, x_start + width, y);
    }
}
void Canvas::DrawCircle(uint32_t color, int x, int y, int radius)
{
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int i = 0;
    int j = radius;
 
    SetPixel(x, y + radius, color);
    SetPixel(x, y - radius, color);
    SetPixel(x + radius, y, color);
    SetPixel(x - radius, y, color);
 
    while(i < j) {
        if(f >= 0) {
            j--;
            ddF_y += 2;
            f += ddF_y;
        }
        i++;
        ddF_x += 2;
        f += ddF_x;
        SetPixel(x + i, y + j, color);
        SetPixel(x - i, y + j, color);
        SetPixel(x + i, y - j, color);
        SetPixel(x - i, y - j, color);
        SetPixel(x + j, y + i, color);
        SetPixel(x - j, y + i, color);
        SetPixel(x + j, y - i, color);
        SetPixel(x - j, y - i, color);
    }
}
void Canvas::DrawFillCircle(uint32_t color, int x, int y, int radius)
{
    this->DrawVerticalLine(color, 2 * radius + 1, x, y - radius);
    this->FillCircleHelper(x, y, radius, 3, 0, color);
}
void Canvas::DrawEllipse(uint32_t color, int x_center, int y_center, int x_radius, int y_radius)
{
    int a = 2 * x_radius;
    int b = 2 * y_radius;
    int b1 = b & 1;
    int dx = 4 * (1 - a) * b * b;
    int dy = 4 * (b1 + 1) * a * a;
    int err = dx + dy + b1 * a * a;
    int e2;
    int y = 0;
    int x = x_radius;
    a *= 8 * a;
    b1 = 8 * b * b;

    while (x >= 0)
    {
        SetPixel(x_center + x, y_center + y, color);
        SetPixel(x_center - x, y_center + y, color);
        SetPixel(x_center - x, y_center - y, color);
        SetPixel(x_center + x, y_center - y, color);
        e2 = 2 * err;
        if (e2 <= dy) { y++; err += dy += a; }
        if (e2 >= dx || 2 * err > dy) { x--; err += dx += b1; }
    }
}

void Canvas::DrawString(Font* font, char* string, int x, int y, uint32_t color)
{
    if(font == 0 || string == 0 || color == Colors::Transparent)
        return;
    
    int xOffset = x;
    int yOffset = y;
    while(*string)
    {
        // Get the character we need to draw for this string
        char c = *string++;

        // Check for newline
        if(c == '\n') {
            xOffset = x;

            // Add the height of the space character. TODO: Update this!
            yOffset += ((uint8_t*)(font->data + font->offsetTable[0]))[1];
            continue;
        }

        // Load data for this char from the font
        const uint8_t* charData = (uint8_t*)(font->data + font->offsetTable[(int)c - 32]);
        const uint8_t width = charData[0];
        const uint8_t height = charData[1];

        // Loop through the complete bitmap and draw the character
        for(uint8_t px = 0; px < width; px++) {
            for(uint8_t py = 0; py < height; py++) {
                // Can be any value between 0 and 255
                uint8_t d = charData[py * width + px + 2];
                
                // This pixel does not need to be drawn
                if(d == 0)
                    continue;

                // This is a full color pixel
                if(d == 255)
                    this->SetPixel(px + xOffset, py + yOffset, color);
                // We need to blend this pixel with the background
                else {
                    Color4 realColor;
                    realColor.c = color;
                    realColor.argb.a = d; // Adjust the alpha component of the color. TODO: Also support full transparent text drawing in the future!

                    this->SetPixel(px + xOffset, py + yOffset, Colors::AlphaBlend(this->GetPixel(px + xOffset, py + yOffset), realColor.c));
                }  
            }
        }

        xOffset += width;
    }
}
void Canvas::DrawCircleHelper(int x, int y, int radius, uint32_t corner, uint32_t color)
{
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int i = 0;
    int j = radius;
 
    while (i < j) {
        if (f >= 0) {
            j--;
            ddF_y += 2;
            f += ddF_y;
        }
        i++;
        ddF_x += 2;
        f += ddF_x;
        if (corner & 0x4) {
            SetPixel(x + i, y + j, color);
            SetPixel(x + j, y + i, color);
        }
        if (corner & 0x2) {
            SetPixel(x + i, y - j, color);
            SetPixel(x + j, y - i, color);
        }
        if (corner & 0x8) {
            SetPixel(x - j, y + i, color);
            SetPixel(x - i, y + j, color);
        }
        if (corner & 0x1) {
            SetPixel(x - j, y - i, color);
            SetPixel(x - i, y - j, color);
        }
    }
}
 
void Canvas::FillCircleHelper(int x, int y, int radius, uint32_t corner, int delta, uint32_t color)
{
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int i = 0;
    int j = radius;
 
    while (i < j) {
        if (f >= 0) {
            j--;
            ddF_y += 2;
            f += ddF_y;
        }
        i++;
        ddF_x += 2;
        f += ddF_x;
 
        if (corner & 0x1) {
            DrawVerticalLine(color, 2 * j + 1 + delta, x + i, y - j);
            DrawVerticalLine(color, 2 * i + 1 + delta, x + j, y - i);
        }
        if (corner & 0x2) {
            DrawVerticalLine(color, 2 * j + 1 + delta, x - i, y - j);
            DrawVerticalLine(color, 2 * i + 1 + delta, x - j, y - i);
        }
    }
}