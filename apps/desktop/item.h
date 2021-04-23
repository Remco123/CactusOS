#ifndef ITEM_H
#define ITEM_H

#include <gui/context.h>
#include <gui/gui.h>
#include <imaging/image.h>
#include <imaging/bmpformat.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

class DesktopItem : Rectangle
{
public:
    Context* context;
    char* label;
    bool drawLabel;
    char* filename;
    uint8_t* iconBuffer;
    DesktopItem(int x, int y, int width, int height);

    void DrawToContext();
};

DesktopItem::DesktopItem(int x, int y, int width, int height)
: Rectangle(width, height, x, y) {
    this->context = GUI::RequestContext(width, height, x, y);
    this->context->canvas->Clear();
    this->context->sharedContextInfo->background = true;
    this->context->sharedContextInfo->supportsTransparency = true;
    this->context->sharedContextInfo->supportsDirtyRects = true;
}

void DesktopItem::DrawToContext()
{
    if(this->iconBuffer)
    {
        Image img = ConvertBMPRaw(this->iconBuffer);
        for(int x = 0; x < img.GetWidth(); x++)
            for(int y = 0; y < img.GetHeight(); y++) {
                uint32_t argb = img[y * img.GetWidth() + x];
                if(argb != 0xFFFFFFFF)
                    this->context->canvas->SetPixel(x, y, argb);
                else
                    this->context->canvas->SetPixel(x, y, 0x00000000);
            }
    }
    if(this->drawLabel) {
        this->context->canvas->DrawRect(0xFF000000, 0, height-20, width-1, 20-1);
        this->context->canvas->DrawString(GUI::defaultFont, this->label, 5, height-17, 0xFFFFFFFF);
    }
    this->context->SetDirtyArea(0, 0, width, height);
}

#endif