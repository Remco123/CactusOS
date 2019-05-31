#include "../init/bmp.h"
#include <vfs.h>
#include <log.h>
#include <gui/canvas.h>

using namespace LIBCactusOS;

uint8_t* LoadBackground(char* fileName)
{
    if(FileExists(fileName))
    {
        uint32_t fileSize = GetFileSize(fileName);
        if(fileSize != -1)
        {
            uint8_t* fileBuf = new uint8_t[fileSize];
            ReadFile(fileName, fileBuf);

            BMPFileHeader* h = (BMPFileHeader*)fileBuf;
            BMPInfoHeader* info = (BMPInfoHeader*)(fileBuf + sizeof(BMPFileHeader));
            Print("Wallpaper Format: w=%d h=%d bpp=%d\n", info->biWidth, info->biHeight, info->biBitCount);
            if(info->biBitCount != 24 || info->biWidth != 1024 || info->biHeight != 768)
                return 0;

            //Allocate buffer for image to return
            uint8_t* imageBuffer = new uint8_t[info->biWidth * info->biHeight * 4];
            //This makes our life a bit easier
            Canvas* imageBufferCanvas = new Canvas(imageBuffer, info->biWidth, info->biHeight);
             
            uint8_t* imageData = (uint8_t*)((unsigned int)fileBuf + h->bfOffBits);

            //Display Image
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

                    uint32_t argb = 0xFF000000 | r << 16 | g << 8 | b;

                    imageBufferCanvas->SetPixel(x, y, argb);
                }
            }
            delete fileBuf;
            delete imageBufferCanvas;

            return imageBuffer;
        }
    }
    return 0;
}