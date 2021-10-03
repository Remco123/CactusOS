#include <imaging/image.h>
#include <log.h>
#include <string.h>
#include <math.h>
#include <vfs.h>
#include <heap.h>

#include <imaging/bmpformat.h>
#include <imaging/jpeg_decoder.h>
#include <imaging/pngformat.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

Image::Image(const int width, const int height)
{
    this->width = width;
    this->height = height;
    if(width > 0 && height > 0)
        this->buffer = new uint32_t[width * height];
}
Image::~Image()
{
    if(this->canvas)
        delete this->canvas;
    
    if(this->buffer)
        delete this->buffer;
}

//////////////////////
// General Functions
//////////////////////

Canvas* Image::GetCanvas()
{
    if(this->canvas == 0)
        this->canvas = new Canvas(this->buffer, this->width, this->height);
    
    return this->canvas;
}

int Image::GetWidth()
{
    return this->width;
}

int Image::GetHeight()
{
    return this->height;
}

uint32_t* Image::GetBufferPtr()
{
    return this->buffer;
}

void Image::DrawTo(Canvas* target, int x, int y)
{
    if(target == 0)
        return;
    
    uint8_t* targetBuffer = (uint8_t*)target->bufferPointer;
    for(int row = 0; row < this->height; row++)
        memcpy((void*)(targetBuffer + (row+y)*target->Width*4 + x*4), (void*)(this->buffer + row*this->width), this->width * 4);
}


//////////////////
// Static Functions
//////////////////

Image* Image::CreateFromFile(const char* filepath, const char* ext)
{
    const char* extension = 0;

    if(ext == 0) { // We need to determine type by extension
        int i = str_IndexOf(filepath, '.');
        if(i == -1) {
            Print("Image path %s not valid\n", filepath);
            return 0;
        }

        // Create extension from filepath
        extension = (char*)(filepath + i + 1);
    }
    else
        extension = ext; // Extension is given



    if(strcmp(extension, "bmp") == 1)
        return ConvertBMP(filepath);
    else if(strcmp(extension, "jpg") == 1 || strcmp(extension, "jpeg") == 1) {
        Print("[JPEG] Converting image file %s\n", filepath);

        if(FileExists((char*)filepath))
        {
            uint32_t fileSize = GetFileSize((char*)filepath);
            if(fileSize != (uint32_t)-1)
            {
                uint8_t* fileBuf = new uint8_t[fileSize];
                ReadFile((char*)filepath, fileBuf);
                
                // Convert JPEG
                Jpeg::Decoder* decoder = new Jpeg::Decoder(fileBuf, fileSize, UserHeap::Malloc, UserHeap::Free);
                if (decoder->GetResult() != Jpeg::Decoder::OK)
                {
                    Print("[JPEG] Error decoding the input file\n");
                    return 0;
                }
                Image* result = new Image(decoder->GetWidth(), decoder->GetHeight());
                const uint8_t* data = decoder->GetImage();
                Print("[JPEG] Conversion Succes! (%dx%d)\n", result->width, result->height);

                // Move pixels to result image
                for(uint32_t pixel = 0; pixel < (uint32_t)(result->width * result->height); pixel++) {
                    const uint8_t r = data[pixel * 3];
                    const uint8_t g = data[pixel * 3 + 1];
                    const uint8_t b = data[pixel * 3 + 2];
                    result->buffer[pixel] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
                
                delete fileBuf;
                delete decoder;
                return result;
            }
        }
    }
    else if(strcmp(extension, "png") == 1) {
        return PNGDecoder::Convert(filepath);
    }
    else
        Print("Could not found a image converter for extension %s\n", extension);
    
    return 0;
}

Image* Image::Resize(Image* source, int newWidth, int newHeight, ResizeMethod method)
{
    if(source == 0) return 0;

    if(source->width == newWidth && source->height == newHeight) // No change in resolution
        return source;
    
    switch(method)
    {
        case NearestNeighbor:
            return ResizeNearestNeighbor(source, newWidth, newHeight);
        case Bilinear:
            return ResizeBilinear(source, newWidth, newHeight);
    }
    return source;
}

///////////
// Resize Implementations
///////////
// http://jankristanto.com/info/nearest-neighbor-interpolation-for-resize-image/
Image* Image::ResizeNearestNeighbor(Image* source, int newWidth, int newHeight)
{
    Image* result = new Image(newWidth, newHeight);
    uint32_t* src = (uint32_t*)source->GetBufferPtr();
    uint32_t* dest = (uint32_t*)result->GetBufferPtr();

    double x_ratio = source->width / (double)newWidth;
	double y_ratio = source->height / (double)newHeight;
	double px, py;
	for (int y = 0; y < newHeight; y++) {
		for (int x = 0; x < newWidth; x++) {
			px = Math::floor((double)x * x_ratio);
			py = Math::floor((double)y * y_ratio);
			dest[(y*newWidth) + x] = src[(uint32_t)((py*source->width) + px)];
		}
	}
    
    return result;
}
// http://tech-algorithm.com/articles/bilinear-image-scaling/
Image* Image::ResizeBilinear(Image* source, int newWidth, int newHeight)
{
    Image* result = new Image(newWidth, newHeight);
    uint8_t* src = (uint8_t*)source->GetBufferPtr();
    uint8_t* dest = (uint8_t*)result->GetBufferPtr();
    int a, b, c, d, x, y, index;

    float x_ratio = ((float)(source->width-1))/newWidth;
    float y_ratio = ((float)(source->height-1))/newHeight;

    float x_diff, y_diff, blue, red, green;
    #if BILINEAR_ALPHA
    float alpha;
    #endif
    
    int offset = 0;
    for (int i = 0; i < newHeight; i++) {
        for (int j = 0; j < newWidth; j++) {
            x = (int)(x_ratio * j);
            y = (int)(y_ratio * i);
            x_diff = (x_ratio * j) - x;
            y_diff = (y_ratio * i) - y;
            index = (y*source->width+x);   

            a = src[index];
            b = src[index+1];
            c = src[index+source->width];
            d = src[index+source->width+1];

            // blue element
            // Yb = Ab(1-w)(1-h) + Bb(w)(1-h) + Cb(h)(1-w) + Db(wh)
            blue = (a&0xff)*(1-x_diff)*(1-y_diff) + (b&0xff)*(x_diff)*(1-y_diff) +
                   (c&0xff)*(y_diff)*(1-x_diff)   + (d&0xff)*(x_diff*y_diff);

            // green element
            // Yg = Ag(1-w)(1-h) + Bg(w)(1-h) + Cg(h)(1-w) + Dg(wh)
            green = ((a>>8)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>8)&0xff)*(x_diff)*(1-y_diff) +
                    ((c>>8)&0xff)*(y_diff)*(1-x_diff)   + ((d>>8)&0xff)*(x_diff*y_diff);

            // red element
            // Yr = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
            red = ((a>>16)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>16)&0xff)*(x_diff)*(1-y_diff) +
                  ((c>>16)&0xff)*(y_diff)*(1-x_diff)   + ((d>>16)&0xff)*(x_diff*y_diff);

            #if BILINEAR_ALPHA
            alpha = ((a>>24)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>24)&0xff)*(x_diff)*(1-y_diff) +
                    ((c>>24)&0xff)*(y_diff)*(1-x_diff)   + ((d>>24)&0xff)*(x_diff*y_diff);

            dest[offset++] = 
                    ((((int)alpha)<<24)&0xff000000) |
                    ((((int)red)<<16)&0xff0000) |
                    ((((int)green)<<8)&0xff00) |
                    ((int)blue);
            #else
            dest[offset++] = 
                    0xff000000 | // hardcode alpha
                    ((((int)red)<<16)&0xff0000) |
                    ((((int)green)<<8)&0xff00) |
                    ((int)blue);
            #endif
        }
    }

	return result;
}