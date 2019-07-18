#ifndef ITEM_H
#define ITEM_H

#include <gui/context.h>
#include <gui/gui.h>
#include "../init/bmp.h"

using namespace LIBCactusOS;

class DesktopItem : Rectangle
{
public:
    Context* context;
    char* label;
    char* filename;
    uint8_t* iconBuffer;
    DesktopItem(int x, int y, int width, int height);

    void DrawToContext();
};

DesktopItem::DesktopItem(int x, int y, int width, int height)
: Rectangle(width, height, x, y) {
    this->context = GUI::RequestContext(width, height, x, y);
    this->context->sharedContextInfo->background = true;
    this->context->sharedContextInfo->supportsTransparency = true;
}

void DesktopItem::DrawToContext()
{
    if(this->iconBuffer)
    {
        BMPFileHeader* h = (BMPFileHeader*)iconBuffer;
        BMPInfoHeader* info = (BMPInfoHeader*)(iconBuffer + sizeof(BMPFileHeader));
        uint8_t* imageData = (uint8_t*)((unsigned int)iconBuffer + h->bfOffBits);

        int alignment = 0;
        alignment = (info->biWidth * 3) % 4;
        if (alignment != 0)
        {
            alignment = 4 - alignment;
        }  

        int offset, rowOffset;
        for (int y = 0; y < info->biHeight; y++)
        {
            rowOffset = (info->biHeight - y - 1) * (info->biWidth * 3 + alignment);

            for (int x = 0; x < info->biWidth; x++)
            {
                offset = rowOffset + x * 3;
                    
                uint32_t b = imageData[offset + 0];
                uint32_t g = imageData[offset + 1];
                uint32_t r = imageData[offset + 2];

                uint32_t argb = 0xFF000000 |
                                r << 16 |
                                g << 8  |
                                b;

                if(argb != 0xFFFFFFFF)
                    this->context->canvas->SetPixel(x, y, argb);
                else
                    this->context->canvas->SetPixel(x, y, 0x00000000);
            }
        }
    }
    this->context->canvas->DrawRect(0xFF000000, 0, height-20, width-1, 20-1);
    this->context->canvas->DrawString(this->label, 5, height-17, 0xFFFFFFFF);
}

#endif