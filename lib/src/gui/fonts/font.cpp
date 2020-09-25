#include <gui/fonts/font.h>

using namespace LIBCactusOS;

void Font::BoundingBox(char* string, int* retW, int* retH)
{
    if(string == 0 || retW == 0 || retH == 0)
        return; // Error with arguments

    if(this->data == 0)
        return; // Not initialized
    
    // Reset variables
    *retW = 0;
    *retH = 0;
    

    int xOffset = 0;
    int yOffset = 0;
    while(*string)
    {
        // Get the character we need to draw for this string
        char c = *string++;

        // Set initial values for first character
        if(xOffset == 0 && yOffset == 0)
            yOffset = ((uint8_t*)(this->data + this->offsetTable[0]))[1];

        // Check for newline
        if(c == '\n') {
            if(xOffset > *retW)
                *retW = xOffset;
            
            xOffset = 0;

            // Add the height of the space character. TODO: Update this!
            yOffset += ((uint8_t*)(this->data + this->offsetTable[0]))[1] + 1;
            continue;
        }

        // Load data for this char from the font
        const uint8_t* charData = (uint8_t*)(this->data + this->offsetTable[(int)c - 32]);
        xOffset += charData[0];
    }

    // Update return values
    // retW might already be correctly set by another line of text
    if(xOffset > *retW)
        *retW = xOffset;
    
    *retH = yOffset;
}