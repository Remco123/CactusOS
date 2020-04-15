#include <installer/textgui.h>
#include <common/types.h>
#include <core/port.h>

using namespace CactusOS;
using namespace CactusOS::common;
using namespace CactusOS::system;
using namespace CactusOS::core;

volatile uint16_t* videoMemory = (uint16_t*)0xC00B8000;

void TextGUI::DisableCursor()
{
    outportb(0x3D4, 0x0A);
	outportb(0x3D5, 0x20);
}
void TextGUI::SetPixel(int x, int y, char color, uint16_t character, char background)
{
    uint16_t attrib = (background << 4) | (color & 0x0F);
    *(videoMemory + (y * VGA_WIDTH + x)) = character | (attrib << 8);
}
void TextGUI::ClearScreen(char color)
{
    for(int y = 0; y < VGA_HEIGHT; y++)
        for(int x = 0; x < VGA_WIDTH; x++)
                SetPixel(x, y, color, ' ');
}
void TextGUI::StatusBar(char* text, int percentage)
{
    for(int x = 0; x < VGA_WIDTH; x++)
        SetPixel(x, VGA_HEIGHT - 1, TEXT_COLOR, ' ', VGA_COLOR_LIGHT_GREY);
    
    DrawString(text, 0, VGA_HEIGHT - 1, TEXT_COLOR, VGA_COLOR_LIGHT_GREY);

    const int barWidth = 20; //Amount of characters for status bar
    const int startX = VGA_WIDTH - barWidth - 1; //Start of progress bar
    int width = ((double)percentage / 100.0) * (double)barWidth;

    SetPixel(startX, VGA_HEIGHT - 1, VGA_COLOR_BLACK, '[', VGA_COLOR_LIGHT_GREY);
    for(int i = 0; i < width; i++)
        SetPixel(startX + i + 1, VGA_HEIGHT - 1, VGA_COLOR_BLACK, '#', VGA_COLOR_LIGHT_GREY);
    
    SetPixel(startX + barWidth, VGA_HEIGHT - 1, VGA_COLOR_BLACK, ']', VGA_COLOR_LIGHT_GREY);
}
void TextGUI::DrawString(char* text, int x, int y, char color, char background)
{
    int px = x;
    int py = y;
    for(int i = 0; text[i] != '\0'; ++i) {
        switch(text[i]) {
            case '\n':
                px = x;
                py += 1;
                break;
            default:
                SetPixel(px, py, color, text[i], background);
                px++;
                break;
        }
    }
}