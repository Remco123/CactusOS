#ifndef __CACTUSOSLIB__IMAGING_IMAGE_H
#define __CACTUSOSLIB__IMAGING_IMAGE_H

#include <types.h>
#include <gui/canvas.h>

namespace LIBCactusOS
{
    namespace Imaging
    {
        // Enable alpha for bilinear image scaling or not?
        #define BILINEAR_ALPHA 1

        enum ResizeMethod
        {
            NearestNeighbor,
            Bilinear
        };

        class Image
        {
        private:
            int width = 0;
            int height = 0;
            Canvas* canvas = 0;

            uint32_t* buffer = 0;
        public:
            // Create new image using a width and height
            Image(const int width, const int height);
            // Destructor
            ~Image();

            // Generate a canvas for this image and return it
            // It will only get generated once for each image and will get destroyed automatically
            Canvas* GetCanvas();

            // Get width of image
            int GetWidth();

            // Get height of image
            int GetHeight();

            // Receive pointer to raw buffer
            uint32_t* GetBufferPtr();

            // Draw this image directly to a canvas
            void DrawTo(Canvas* target, int x = 0, int y = 0);

            // Create image from a file present on disk
            static Image* CreateFromFile(const char* filepath, const char* ext = 0);

            // Resize source image and return result
            static Image* Resize(Image* source, int newWidth, int newHeight, ResizeMethod method = NearestNeighbor);
        private:
            static Image* ResizeNearestNeighbor(Image* source, int newWidth, int newHeight);
            static Image* ResizeBilinear(Image* source, int newWidth, int newHeight);
        };
    }
}

#endif