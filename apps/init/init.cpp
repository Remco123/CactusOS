#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include "bmp.h"
#include "progress.h"

using namespace LIBCactusOS;

char* path = "B:\\boot.bmp";

int main()
{
    Log(Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() == SYSCALL_RET_SUCCES)
    {
        DirectGUI::Clear(0xFFFFFFFF);

        int x_p = WIDTH/2 - 100;
        int y_p = HEIGHT/2 - 200;

        ProgressBar* bar = new ProgressBar(x_p, y_p + 250, 200, 10);
        bar->SetValue(0);

        Log(Info, "Loading Background");
        if(FileExists(path))
        {
            int fileSize = GetFileSize(path);
            if(fileSize != -1)
            {
                uint8_t* fileBuf = new uint8_t[fileSize];
                ReadFile(path, fileBuf);

                Log(Info, "Parsing bmp image...");

                BMPFileHeader* h = (BMPFileHeader*)fileBuf;

                BMPInfoHeader* info = (BMPInfoHeader*)(fileBuf + sizeof(BMPFileHeader));

                Print("BMP Format: w=%d h=%d bpp=%d\n", info->biWidth, info->biHeight, info->biBitCount);

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

                        uint32_t argb = 0xFF000000 |
                                        r << 16 |
                                        g << 8  |
                                        b;

                        DirectGUI::SetPixel(x + x_p, y + y_p, argb);
                    }
                }
            }
        }
        
        Print("Launched test process with status %d\n", Process::Run("B:\\apps\\test.bin"));

        bar->SetValue(80);
    }    

    return 0;
}