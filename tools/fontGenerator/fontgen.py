# import required classes
 
from PIL import Image, ImageDraw, ImageFont
from time import sleep
import sys

# Array that holds a collection of data for each character
characterData = []

# Print iterations progress
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

def ProcessCharacter(char, fontengine):
    global characterData

    msg = ''.join(char)
    charsize = font.getsize(msg)

    # create new image for this character
    image = Image.new(mode = "RGBA", size = charsize)

    # initialise the drawing context with
    # the image object as background
    draw = ImageDraw.Draw(image)

    # Set coordinates
    (x, y) = (0, 0)
    color = 'rgb(0, 0, 0)' # black color

    # draw the message on the background
    draw.text((x, y), msg, fill=color, font=font)

    width,height = charsize

    # create new bytearray for storage
    data = bytearray(2 + width * height)

    # Fill in first 2 data values with width and height of character
    data[0] = width
    data[1] = height

    # Fill in data by iterating trough all individial pixels
    for x in range(width):
        for y in range(height):
            pix = image.getpixel((x,y))
            data[y * width + x + 2] = pix[3] # Get alpha component only from tupple

    # Add this dataset to the global list
    characterData.append(data)


# Start of main program

if(len(sys.argv) == 1):
    print('Missing argument! Exiting application')
    exit()

sizeindots = int(sys.argv[1])
headersize = 392

# Create font for drawing the text
font = ImageFont.truetype('tools/fontGenerator/font.ttf', size=sizeindots)

fontname = font.getname()[0]

print('Parsing font ' + fontname)

# Show initial progressbar
printProgressBar(0, 127-32, prefix = 'Progress:', suffix = 'Complete', length = 50)

# To hold progress
d = 0

# Loop through all characters
for c in (chr(i) for i in range(32,127)):
    ProcessCharacter(c, font)
    printProgressBar(d + 1, 127-32, prefix = 'Progress:', suffix = 'Complete -> ' + c, length = 50)
    d += 1

# Print sumary of parsing
print('Got a total of {0} characters in array'.format(len(characterData)))


#    // Header of a CactusOS Font File (CFF)
#    struct CFFHeader
#    {
#        uint32_t Magic;                     // Magic number containing 0xCFF
#        uint8_t  Version;                   // Version of this font file, propably 1
#        uint16_t FontSize;                  // Size of font in dots
#        uint32_t FontNameOffset;            // Offset in file data where font name is stored
#
#        uint32_t CharacterOffsets[127-32];  // Table which contains offsets to the data for each character
#    };


# Start saving to file
f = open("isofiles/fonts/" + fontname + str(sizeindots) + '.cff', "wb")

print('Writing header....')

# Write header
f.write((0xCFF).to_bytes(4, 'little', signed=False))
f.write((0x1).to_bytes(1, 'little', signed=False))
f.write((sizeindots).to_bytes(2, 'little', signed=False))
f.write((headersize-1).to_bytes(4, 'little', signed=False))

# Write offsets
charDataOffset = headersize + len(fontname)
for i in range(len(characterData)):
    f.write((charDataOffset).to_bytes(4, 'little', signed=False))
    charDataOffset += len(characterData[i])

# Write fontname after header
f.write(bytes(fontname + '\0', 'utf-8'))

print('Writing character data....')

# Write actual data
for i in range(len(characterData)):
    f.write(characterData[i])

print('Done! Filesize ~= {0} Kb'.format(int(f.tell() / 1024)))

# Finally close the file
f.close()