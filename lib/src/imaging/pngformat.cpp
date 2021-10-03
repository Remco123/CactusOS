#include <imaging/pngformat.h>
#include <bitreader.h>
#include <log.h>
#include <string.h>
#include <math.h>
#include <vfs.h>
#include <heap.h>

using namespace LIBCactusOS;
using namespace LIBCactusOS::Imaging;

#define BYTES_PER_PIXEL 4

static const uint32_t lengthExtraBits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, //257 - 264
	1, 1, 1, 1,             //265 - 268
	2, 2, 2, 2,             //269 - 273
	3, 3, 3, 3,             //274 - 276
	4, 4, 4, 4,             //278 - 280
	5, 5, 5, 5,             //281 - 284
	0                       //285
};

static const uint32_t lengthBase[] = {
	3, 4, 5, 6, 7, 8, 9, 10, //257 - 264
	11, 13, 15, 17,          //265 - 268
	19, 23, 27, 31,          //269 - 273
	35, 43, 51, 59,          //274 - 276
	67, 83, 99, 115,         //278 - 280
	131, 163, 195, 227,      //281 - 284
	258                      //285
};

static const uint32_t distanceBase[] = {
	1, 2, 3, 4,    //0-3
	5, 7,          //4-5
	9, 13,         //6-7
	17, 25,        //8-9
	33, 49,        //10-11
	65, 97,        //12-13
	129, 193,      //14-15
	257, 385,      //16-17
	513, 769,      //18-19
	1025, 1537,    //20-21
	2049, 3073,    //22-23
	4097, 6145,    //24-25
	8193, 12289,   //26-27
	16385, 24577,  //28-29
	0   , 0        //30-31, error, shouldn't occur
};

static const uint32_t distanceExtraBits[] = {
	0, 0, 0, 0, //0-3
	1, 1,       //4-5
	2, 2,       //6-7
	3, 3,       //8-9
	4, 4,       //10-11
	5, 5,       //12-13
	6, 6,       //14-15
	7, 7,       //16-17
	8, 8,       //18-19
	9, 9,       //20-21
	10, 10,     //22-23
	11, 11,     //24-25
	12, 12,     //26-27
	13, 13,     //28-29
	0 , 0       //30-31 error, they shouldn't occur
};

static const uint32_t codeLengthCodesOrder[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};


void printBT(const char* prefix, const HuffmanNode* node, bool isLeft)
{
    if( node != nullptr )
    {
        Print("%s%s%d\n", prefix, (isLeft ? "├──" : "└──" ), node->symbol);

        // enter the next tree level - left and right branch
        printBT(str_Combine((char*)prefix, isLeft ? (char*)"│   " : (char*)"    "), node->left, true);
        printBT(str_Combine((char*)prefix, isLeft ? (char*)"│   " : (char*)"    "), node->right, false);
    }
}

void printBT(const HuffmanNode* node)
{
    printBT("", node, false);    
}

Image* PNGDecoder::Convert(const char* filepath)
{
    Print("[PNG] Converting image file %s\n", filepath);

    if(FileExists((char*)filepath))
    {
        uint32_t fileSize = GetFileSize((char*)filepath);
        if(fileSize != (uint32_t)-1)
        {
            uint8_t* fileBuf = new uint8_t[fileSize];
            ReadFile((char*)filepath, fileBuf);
            
            Image* result = ConvertRAW(fileBuf);
            delete fileBuf;
            return result;
        }
    }

    Print("[PNG] Error processing file %s\n", filepath);
    return 0;
}

Image* PNGDecoder::ConvertRAW(const uint8_t* rawData)
{
    const uint8_t signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

    IHDRChunk* ihdr = 0;
    List<uint8_t*> imgDataPtrs;
    List<uint32_t> imgDataLens;
    uint8_t* dataPtr = (uint8_t*)rawData;

    if(memcmp(dataPtr, signature, 8) != 0)
        return 0;

    // Move past the signature
    dataPtr += 8;

    // Start reading chunks
    while(1) {
        PNGChunk* chunk = (PNGChunk*)dataPtr;
        uint32_t dataLength = __builtin_bswap32(chunk->length);

        // Check for chunk type
        if(memcmp(chunk->type, "IHDR", 4) == 0) {
            // Put pointer to the right data
            ihdr = (IHDRChunk*)(dataPtr + sizeof(PNGChunk));
            
            // Perform byteswap for width and height values
            ihdr->width = __builtin_bswap32(ihdr->width);
            ihdr->height = __builtin_bswap32(ihdr->height);
            
            // Perform some sanity checking
            if(ihdr->compression != 0) {
                Log(Error, "[PNG] Compression method is not 0, can not parse this image");
                return 0;
            }
            if(ihdr->filter != 0) {
                Log(Error, "[PNG] Filter method is not 0, can not parse this image");
                return 0;
            }

            // Check if we can read this type of image file
            if(ihdr->colorType != 6) {
                Log(Error, "[PNG] Color type is not 6, can not parse this image");
                return 0;
            }
            if(ihdr->bits != 8) {
                Log(Error, "[PNG] Bits is not 8, can not parse this image");
                return 0;
            }
            if(ihdr->interlace != 0) {
                Log(Error, "[PNG] Interlace method is not 0, can not parse this image");
                return 0;
            }
        }
        else if(memcmp(chunk->type, "IDAT", 4) == 0) {
            // Add a pointer to this chunk of data in the list
            imgDataPtrs.push_back(dataPtr + sizeof(PNGChunk));
            imgDataLens.push_back(dataLength);
        }
        // Check for end of chunks
        else if(memcmp(chunk->type, "IEND", 4) == 0)
            break;
        
        // Go to next chunk
        dataPtr += sizeof(uint32_t) * 3 + dataLength; 
    }

    // Now we need to create one chuck of data for the decompressor to use
    // First calculate the total lenght of data
    uint32_t IDATLength = 0;
    for(uint32_t len : imgDataLens)
        IDATLength += len;

    // Now allocate a buffer for this complete datablock
    uint8_t* IDAT = new uint8_t[IDATLength];

    // And copy all the blocks to this buffer
    uint32_t offset = 0;
    for(int i = 0; i < imgDataPtrs.size(); i++) {
        memcpy(IDAT + offset, imgDataPtrs[i], imgDataLens[i]);
        offset += imgDataLens[i];
    }

    // Decompress image data
    Vector<uint8_t>* imageData = ZLIBDecompressor::Decompress(IDAT);
    uint8_t* imageDataRaw = imageData->data();

    // We don't need this anymore
    delete IDAT;

    // Create resulting image
    Image* result = new Image(ihdr->width, ihdr->height);
    uint8_t* recon = (uint8_t*)result->GetBufferPtr();
    uint32_t reconIndex = 0;
    uint32_t stride = ihdr->width * BYTES_PER_PIXEL;

    uint32_t index = 0;
    for(uint32_t r = 0; r < ihdr->height; r++)
    {
        uint8_t filterType = imageDataRaw[index++];
        uint8_t reconX = 0;
        for(uint32_t c = 0; c < stride; c++)
        {
            uint8_t filtX = imageDataRaw[index++];
            if(filterType == 0)
                reconX = filtX;
            else if(filterType == 1)
                reconX = filtX + Recon_a(recon, stride, r, c);
            else if(filterType == 2)
                reconX = filtX + Recon_b(recon, stride, r, c);
            else if(filterType == 3)
                reconX = filtX + ((Recon_a(recon, stride, r, c) + Recon_b(recon, stride, r, c)) / 2);
            else if(filterType == 4)
                reconX = filtX + PaethPredictor(recon, Recon_a(recon, stride, r, c), Recon_b(recon, stride, r, c), Recon_c(recon, stride, r, c));
            else {
                Print("[PNGDecoder] invalid filter type %d\n", filterType);
            }
            
            recon[reconIndex] = reconX & 0xFF;
            reconIndex++;
        } 
    }

    // Convert to right pixel format
    for(uint32_t i = 0; i < (result->GetHeight() * stride); i += 4)
    {
        const uint8_t a = recon[i + 3];
        const uint8_t b = recon[i + 2];
        const uint8_t g = recon[i + 1];
        const uint8_t r = recon[i + 0];

        recon[i + 3] = a;
        recon[i + 2] = r;
        recon[i + 1] = g;
        recon[i + 0] = b;
    }    

    delete imageData;
    return result;
}

uint8_t PNGDecoder::PaethPredictor(uint8_t* recon, uint8_t a, uint8_t b, uint8_t c)
{
    int p = a + b - c;
    int pa = Math::Abs(p - a);
    int pb = Math::Abs(p - b);
    int pc = Math::Abs(p - c);
    uint8_t pr = 0;
    
    if((pa <= pb) && (pa <= pc))
        pr = a;
    else if(pb <= pc)
        pr = b;
    else
        pr = c;
    
    return pr;
}
uint8_t PNGDecoder::Recon_a(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c)
{
    if(c >= BYTES_PER_PIXEL)
        return recon[r * stride + c - BYTES_PER_PIXEL];
    return 0;
}
uint8_t PNGDecoder::Recon_b(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c)
{
    if(r > 0)
        return recon[(r-1) * stride + c];
    return 0;
}
uint8_t PNGDecoder::Recon_c(uint8_t* recon, uint32_t stride, uint32_t r, uint32_t c)
{
    if((r > 0) && (c >= BYTES_PER_PIXEL))
        return recon[(r - 1) * stride + c - BYTES_PER_PIXEL];
    return 0;
}















Vector<uint8_t>* ZLIBDecompressor::Decompress(uint8_t* input)
{
    /*
        Manely checking that this buffer is supported and valid, main function is the inflate method
    */

    BitReader* reader = new BitReader(input);

    uint8_t CMF = reader->ReadByte();
    uint8_t method = CMF & 0xF;
    if(method != 8) return 0; // Only CM=8 is supported

    uint8_t CINFO = (CMF >> 4) & 0xF;
    if(CINFO > 7) return 0; // CINFO must be smaller that 8

    uint8_t FLG = reader->ReadByte();
    if(((CMF * 256 + FLG) % 31) != 0) return 0; // CMF+FLG checksum not correct

    uint8_t FDICT = (FLG >> 5) & 1;
    if(FDICT != 0) return 0; // FDICT is not supported

    // Perform actual decompression
    Vector<uint8_t>* result = ZLIBDecompressor::Inflate(reader);
    
    // Ignored for now
    uint32_t adler32 = reader->ReadBytes<uint32_t>(4);

    delete reader;
    return result;
}
Vector<uint8_t>* ZLIBDecompressor::Inflate(BitReader* reader)
{
    uint8_t endOfBlocks = 0;
    Vector<uint8_t>* buffer = new Vector<uint8_t>();
    while (!endOfBlocks) 
    {
        endOfBlocks = reader->ReadBit();
        uint8_t blockType = reader->ReadBits<uint8_t>(2);

        //Print("[ZLIBDecompressor] Blocktype %d\n", blockType);
        if(blockType == 0) {
            ZLIBDecompressor::InflateBlockNoCompression(reader, buffer);
        }
        else if(blockType == 1) {
            ZLIBDecompressor::InflateBlockStatic(reader, buffer);
        }
        else if(blockType == 2) {
            ZLIBDecompressor::InflateBlockDynamic(reader, buffer);
        }
        else {
            Print("[ZLIBDecompressor] Invalid blocktype %d\n", blockType);
            return 0;
        }
    }

    return buffer;
}
void ZLIBDecompressor::InflateBlockNoCompression(BitReader* reader, Vector<uint8_t>* target)
{
    uint16_t len = reader->ReadBytes<uint16_t>(2);
    uint16_t nlen = reader->ReadBytes<uint16_t>(2);

    for(uint16_t i = 0; i < len; i++)
        target->push_back(reader->ReadByte());
}
uint32_t ZLIBDecompressor::DecodeSymbol(BitReader* reader, HuffmanTree* tree)
{
    HuffmanNode* node = tree->root;

    while(node->left || node->right) {
        uint8_t b = reader->ReadBit();
        if(b) node = node->right;
        else node = node->left;
    }
    return node->symbol;
}
void ZLIBDecompressor::InflateBlockData(BitReader* reader, HuffmanTree* literalLengthTree, HuffmanTree* distanceTree, Vector<uint8_t>* target)
{
    while(true)
    {
        uint32_t sym = DecodeSymbol(reader, literalLengthTree);
        if (sym <= 255) { // Literal byte
            target->push_back(sym & 0xFF);
        }
        else if(sym == 256) { // End of block
            return;
        }
        else { // <length, backward distance> pair
            sym -= 257;
            uint32_t length = reader->ReadBits<uint32_t>(lengthExtraBits[sym]) + lengthBase[sym];
            uint32_t distSym = DecodeSymbol(reader, distanceTree);
            uint32_t dist = reader->ReadBits<uint32_t>(distanceExtraBits[distSym]) + distanceBase[distSym];
            
            dist = target->Size() - dist;
            for(uint32_t i = 0; i < length; i++, dist++) {
                target->push_back(target->GetAt(dist));
            }
        }
    }
}
HuffmanTree* ZLIBDecompressor::BitListToTree(Vector<uint32_t>* bitList, Vector<uint8_t>* alphabet)
{
    uint32_t maxBits = 0;
    for(uint32_t d : *bitList)
        if(d > maxBits)
            maxBits = d;
    
    uint32_t* blCount = new uint32_t[(maxBits+1)];
    memset(blCount, 0, (maxBits + 1) * sizeof(uint32_t));
    for(uint32_t i = 0; i < (maxBits+1); i++) {
        // Count occurencies in bitList
        for(uint32_t item : *bitList)
            if(i == item && item != 0)
                blCount[i] += 1;
    }

    uint32_t* nextCode = new uint32_t[(maxBits+1)];
    memset(nextCode, 0, (maxBits + 1) * sizeof(uint32_t));
    nextCode[0] = 0;
    nextCode[1] = 0;
    for(uint32_t bits = 2; bits < (maxBits+1); bits++) {
        nextCode[bits] = ((nextCode[bits-1] + blCount[bits-1]) << 1);
    }
    
    HuffmanTree* result = new HuffmanTree();
    for(int i = 0; i < alphabet->Size(); i++) {
        uint32_t c = i;
        uint32_t bitLen = (i < bitList->Size()) ? bitList->GetAt(i) : 0;
        if(bitLen) {
            result->Insert(nextCode[bitLen], bitLen, c);
            nextCode[bitLen] += 1;
        }
    }
    delete blCount;
    delete nextCode;
    return result;
}
DecodeTreesResult ZLIBDecompressor::DecodeTrees(BitReader* reader)
{
    // The number of literal/length codes
    uint32_t HLIT = reader->ReadBits<uint8_t>(5) + 257;

    // The number of distance codes
    uint32_t HDIST = reader->ReadBits<uint8_t>(5) + 1;

    // The number of code length codes
    uint32_t HCLEN = reader->ReadBits<uint8_t>(4) + 4;

    // Create result container
    DecodeTreesResult result;

    // Read code lengths for the code length alphabet
    uint32_t codeLengthTreeBitList[19];
    for(int i = 0; i < 19; i++)
        codeLengthTreeBitList[i] = 0;
    
    for(uint32_t i = 0; i < HCLEN; i++)
        codeLengthTreeBitList[codeLengthCodesOrder[i]] = reader->ReadBits<uint8_t>(3);

    Vector<uint32_t> codeLengthBitList;
    Vector<uint8_t> codeLengthAlphaList;
    for(int i = 0; i < 19; i++) {
        codeLengthBitList.push_back(codeLengthTreeBitList[i]);
        codeLengthAlphaList.push_back(i);
    }

    // Construct code length tree
    HuffmanTree* codeLengthTree = BitListToTree(&codeLengthBitList, &codeLengthAlphaList);

    // Read literal/length + distance code length list
    Vector<uint32_t> bl;
    while (bl.Size() < HLIT + HDIST)
    {
        uint32_t sym = DecodeSymbol(reader, codeLengthTree);
        if((sym >= 0) && (sym <= 15)) { // literal value
            bl.push_back(sym);
        }
        else if(sym == 16) {
            // copy the previous code length 3..6 times.
            // the next 2 bits indicate repeat length ( 0 = 3, ..., 3 = 6 )
            uint32_t prev_code_length = bl.GetAt(bl.Size() - 1);
            uint32_t repeat_length = reader->ReadBits<uint8_t>(2) + 3;
            for(uint32_t i = 0; i < repeat_length; i++)
                bl.push_back(prev_code_length);
        }
        else if(sym == 17) {
            // repeat code length 0 for 3..10 times. (3 bits of length)
            uint32_t repeat_length = reader->ReadBits<uint8_t>(3) + 3;
            for(uint32_t i = 0; i < repeat_length; i++)
                bl.push_back(0);
        }
        else if(sym == 18) {
            // repeat code length 0 for 11..138 times. (7 bits of length)
            uint32_t repeat_length = reader->ReadBits<uint8_t>(7) + 11;
            for(uint32_t i = 0; i < repeat_length; i++)
                bl.push_back(0);
        }
        else
            Log(Error, "[ZLIBDecompressor] Invalid symbol");
    }
    delete codeLengthTree;

    // Construct trees
    Vector<uint32_t> blTemp;
    Vector<uint8_t> alphaTemp;
    for(uint32_t i = 0; i < HLIT; i++)
        blTemp.push_back(bl[i]);
    
    for(int i = 0; i < 286; i++)
        alphaTemp.push_back(i);
    
    // Create first tree for the literal lengths
    result.literalLengthTree = BitListToTree(&blTemp, &alphaTemp);

    // Reset lists for re-use
    blTemp.clear();
    alphaTemp.clear();

    for(int i = HLIT; i < bl.Size(); i++)
        blTemp.push_back(bl[i]);
    
    for(int i = 0; i < 30; i++)
        alphaTemp.push_back(i);

    // Create second list
    result.distanceTree = BitListToTree(&blTemp, &alphaTemp);

    // And exit function
    return result;
}
void ZLIBDecompressor::InflateBlockDynamic(BitReader* reader, Vector<uint8_t>* target)
{
    DecodeTreesResult ret = ZLIBDecompressor::DecodeTrees(reader);

    InflateBlockData(reader, ret.literalLengthTree, ret.distanceTree, target);
    delete ret.literalLengthTree;
    delete ret.distanceTree;
}

void ZLIBDecompressor::InflateBlockStatic(BitReader* reader, Vector<uint8_t>* target)
{
    static HuffmanTree* literalLengthTree = 0;
    static HuffmanTree* distanceTree = 0;

    if(literalLengthTree == 0) {
        Vector<uint32_t> bl1;
        
        Vector<uint8_t> alphaTemp;
        for(int i = 0; i < 286; i++)
            alphaTemp.push_back(i);

        for(int i = 0; i < 144; i++)
            bl1.push_back(8);
        for(int i = 144; i < 256; i++)
            bl1.push_back(9);
        for(int i = 256; i < 280; i++)
            bl1.push_back(7);
        for(int i = 280; i < 288; i++)
            bl1.push_back(8);
        
        literalLengthTree = BitListToTree(&bl1, &alphaTemp);

        bl1.clear();
        alphaTemp.clear();
        for(int i = 0; i < 30; i++) {
            bl1.push_back(5);
            alphaTemp.push_back(i);
        }

        distanceTree = BitListToTree(&bl1, &alphaTemp);
    }

    ZLIBDecompressor::InflateBlockData(reader, literalLengthTree, distanceTree, target);
}