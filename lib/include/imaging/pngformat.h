#ifndef __CACTUSOSLIB__IMAGING__PNGIMAGE_H
#define __CACTUSOSLIB__IMAGING__PNGIMAGE_H

#include <imaging/image.h>
#include <bitreader.h>
#include <list.h>
#include <string.h>

namespace LIBCactusOS
{
    namespace Imaging
    {
        #define ZLIB_BUFFER_SIZE 10_MB

        struct PNGChunk
        {
            uint32_t length;
            uint8_t type[4];
            //uint8_t data;
            //uint32_t crc;
        } __attribute__((packed));

        struct IHDRChunk
        {
            uint32_t width;
            uint32_t height;
            uint8_t bits;
            uint8_t colorType;
            uint8_t compression;
            uint8_t filter;
            uint8_t interlace;
        } __attribute__((packed));

        class PNGBuffer
        {
        public:
            uint32_t index = 0;
        public:
            uint8_t* buffer = 0;
            PNGBuffer(uint32_t size)
            {
                buffer = new uint8_t[size];
                memset(buffer, 0, size);
            }
            ~PNGBuffer() { }

            void Add(uint8_t byte)
            {
                buffer[index++] = byte;
            }
            uint8_t Get(uint32_t i)
            {
                return buffer[i];
            }
            uint32_t Size()
            {
                return index;
            }
        };

        class PNGDecoder
        {
        private:
            static uint8_t PaethPredictor(uint8_t* recon, uint8_t a, uint8_t b, uint8_t c);
            static uint8_t Recon_a(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c);
            static uint8_t Recon_b(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c);
            static uint8_t Recon_c(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c);
        public:
            // Convert image file into image buffer
            static Image Convert(const char* filepath);

            // Create image from array of bytes in png format
            static Image ConvertRAW(const uint8_t* rawData);
        };

        // Represents a single node in the huffman tree
        class HuffmanNode
        {
        public:
            uint32_t symbol = 0;
            HuffmanNode* left = 0;
            HuffmanNode* right = 0;

            HuffmanNode()
            {
                this->symbol = 0;
                this->left = 0;
                this->right = 0;
            }
        };
        class HuffmanTree
        {
        public:
            HuffmanNode* root = 0;

            HuffmanTree()
            {
                this->root = new HuffmanNode();
            }
            void Insert(uint32_t codeWord, uint32_t n, uint32_t symbol)
            {
                HuffmanNode* node = this->root;
                HuffmanNode* nextNode = 0;
                for(int i = n - 1; i >= 0; i--) {
                    uint32_t b = codeWord & (1 << i);
                    if(b) {
                        nextNode = node->right;
                        if(nextNode == 0) {
                            node->right = new HuffmanNode();
                            nextNode = node->right;
                        }
                    }
                    else {
                        nextNode = node->left;
                        if(nextNode == 0) {
                            node->left = new HuffmanNode();
                            nextNode = node->left;
                        }
                    }
                    node = nextNode;
                }
                node->symbol = symbol;
            }
        };

        struct DecodeTreesResult
        {
            HuffmanTree* literalLengthTree;
            HuffmanTree* distanceTree;
        };

        // Class used to decompress ZLIB data such as png and zip files
        class ZLIBDecompressor
        {
        private:
            // Reads data from a non compression block
            static void InflateBlockNoCompression(BitReader* reader, PNGBuffer* target);
            
            // Reads data from dynamic block
            static void InflateBlockDynamic(BitReader* reader, PNGBuffer* target);

            // Reads data from static block
            static void InflateBlockStatic(BitReader* reader, PNGBuffer* target);

            // Decodes one symbol from bitstream using a HuffmanTree
            static uint32_t DecodeSymbol(BitReader* reader, HuffmanTree* tree);

            static void InflateBlockData(BitReader* reader, HuffmanTree* literalLengthTree, HuffmanTree* distanceTree, PNGBuffer* target);

            static HuffmanTree* BitListToTree(List<uint32_t>* bitList, List<uint8_t>* alphabet);
        
            static DecodeTreesResult DecodeTrees(BitReader* reader);
        public:
            // Perform decompression on input and return the complete set of data
            static uint8_t* Decompress(uint8_t* input, uint32_t* lenOut = 0);

            // Perform the actual inflation of the DEFLATE block
            static uint8_t* Inflate(BitReader* reader, uint32_t* lenOut = 0);
        };
    }
}

#endif