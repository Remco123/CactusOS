#ifndef __CACTUSOSLIB__IMAGING__BMPIMAGE_H
#define __CACTUSOSLIB__IMAGING__BMPIMAGE_H

#include <imaging/image.h>

namespace LIBCactusOS
{
    namespace Imaging
    {
        struct BMPFileHeader {
            uint16_t fileType;
            uint32_t fileSize;
            uint16_t Reserved1;
            uint16_t Reserved2;
            uint32_t dataOffset;
        } __attribute__((packed));

        struct BMPInfoHeader {
            uint32_t headerSize;
            int32_t Width;
            int32_t Height;
            uint16_t Planes;
            uint16_t BitCount;
            uint32_t Compression;
            uint32_t SizeImage;
            int32_t XPelsPerMeter;
            int32_t YPelsPerMeter;
            uint32_t ClrUsed;
            uint32_t ClrImportant;
        } __attribute__((packed));

        struct BMPColorHeader {
            uint32_t redMask;
            uint32_t greenMask;
            uint32_t blueMask;
            uint32_t alphaMask;
            uint32_t colorSpaceType;
            uint32_t unused[16];
        } __attribute__((packed));

        // Convert image file into image buffer
        Image* ConvertBMP(const char* filepath);
        // Create image from array of bytes in bmp format
        Image* ConvertBMPRaw(const uint8_t* rawData);   
    }
}

#endif