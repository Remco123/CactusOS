#include <log.h>
#include <api.h>
#include <types.h>
#include <vfs.h>
#include <syscall.h>
#include <gui/directgui.h>
#include <string.h>
#include <new.h>
#include <proc.h>
#include <ipc.h>
#include <time.h>
#include "bmp.h"
#include "progress.h"

using namespace LIBCactusOS;

char* path = "B:\\boot.bmp";

int main()
{
    Log(Info, "Init process started!");

    if (DirectGUI::RequestFramebuffer() != SYSCALL_RET_SUCCES)
        return -1;

    DirectGUI::Clear(0xFFFFFFFF);

    int x_p = WIDTH/2 - 100;
    int y_p = HEIGHT/2 - 200;

    ProgressBar* bar = new ProgressBar(x_p, y_p + 250, 200, 10);
    bar->SetValue(0);

    Log(Info, "Loading Boot Logo");
    if(FileExists(path))
    {
        uint32_t fileSize = GetFileSize(path);
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

    bar->SetValue(70);

    int s = Process::Run("B:\\apps\\composit.bin");
    Print("Launched compositor with status %d\n", s);

    Time::Sleep(100);

    int s2 = Process::Run("B:\\apps\\calc.bin");
    Print("Launched Calculator with status %d\n", s2);

    Time::Sleep(100);

    int s3 = Process::Run("B:\\apps\\clock.bin");
    Print("Launched Clock with status %d\n", s3);

    bar->SetValue(100); 

    return 0;
}