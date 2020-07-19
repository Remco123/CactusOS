#ifndef __LIBCACTUSOS__GUI__FONTS__FONTPARSER_H
#define __LIBCACTUSOS__GUI__FONTS__FONTPARSER_H

#include <types.h>
#include <gui/fonts/font.h>

namespace LIBCactusOS
{
    // Header of a CactusOS Font File (CFF)
    struct CFFHeader
    {
        uint32_t Magic;                     // Magic number containing 0xCFF
        uint8_t  Version;                   // Version of this font file, propably 1
        uint16_t FontSize;                  // Size of font in dots
        uint32_t FontNameOffset;            // Offset in file data where font name is stored

        uint32_t CharacterOffsets[127-32];  // Table which contains offsets to the data for each character
    } __attribute__((packed));

    class FontParser
    {
    public:
        static Font* FromFile(char* filename);
    };
}

#endif