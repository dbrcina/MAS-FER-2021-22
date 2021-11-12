#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_WIDTH 2
#define BLOCK_HEIGHT 2
#define BLOCK_SIZE BLOCK_WIDTH * BLOCK_HEIGHT

typedef struct {
    uint8_t val;
} PixelGS8;

typedef struct {
    char type[3];
    uint16_t width, height, maxVal;
    PixelGS8 *data;
} ImagePGM;

void skipComments(FILE *file) {
    unsigned char c;
    while ((c = getc(file)) == '#') {
        while (getc(file) != '\n');
    }
    ungetc(c, file);
}

ImagePGM readPGMImage(const char *pgmFile) {
    FILE *file = fopen(pgmFile, "rb");
    if (file == NULL) {
        perror("readPGMImage()::fopen()");
        exit(EXIT_FAILURE);
    }

    skipComments(file);

    char magicNumber[3];
    if (fscanf(file, "%s\n", magicNumber) != 1) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing 'magic number' failed!\n");
        exit(EXIT_FAILURE);
    }

    skipComments(file);

    uint16_t width;
    if (fscanf(file, "%hu\n", &width) != 1) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing width failed!\n");
        exit(EXIT_FAILURE);
    }

    skipComments(file);

    uint16_t height;
    if (fscanf(file, "%hu\n", &height) != 1) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing height failed!\n");
        exit(EXIT_FAILURE);
    }

    skipComments(file);

    uint16_t maxVal;
    if (fscanf(file, "%hu\n", &maxVal) != 1) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing maxVal failed!\n");
        exit(EXIT_FAILURE);
    }

    uint32_t size = width * height;
    PixelGS8 *data = (PixelGS8 *) malloc(sizeof(PixelGS8) * size);
    if (fread(data, sizeof(PixelGS8), size, file) != size) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing data failed!\n");
        exit(EXIT_FAILURE);
    }

    fclose(file);

    ImagePGM image = {.width=width, .height=height, .maxVal=maxVal, .data=data};
    strcpy(image.type, magicNumber);
    return image;
}

PixelGS8 *getPGMImageBlock(ImagePGM *img, uint16_t blockIndex) {
    PixelGS8 *block = (PixelGS8 *) malloc(sizeof(PixelGS8) * BLOCK_SIZE);
    uint32_t xBlockCount = img->width / BLOCK_WIDTH;
    uint32_t yBlockCount = img->height / BLOCK_HEIGHT;
    uint32_t x = blockIndex % xBlockCount * BLOCK_WIDTH;
    uint32_t y = blockIndex / yBlockCount * BLOCK_HEIGHT * img->width;
    uint32_t yOffset = y;
    uint32_t counter = 0;
    for (size_t i = 0; i < BLOCK_HEIGHT; ++i) {
        uint32_t xOffset = x;
        for (size_t j = 0; j < BLOCK_WIDTH; ++j) {
            block[counter++] = img->data[yOffset + xOffset];
            ++xOffset;
        }
        yOffset += img->width;
    }
    return block;
}

int main(int argc, char *argv[]) {
    uint16_t blockNumber = atoi(argv[1]);
    ImagePGM currentImg;
    ImagePGM previousImg;
    if (argc < 3) {
        currentImg = readPGMImage("lenna1.pgm");
        previousImg = readPGMImage("lenna.pgm");
    } else {
        currentImg = readPGMImage(argv[2]);
        previousImg = readPGMImage(argv[3]);
    }

    free(currentImg.data);
    free(previousImg.data);
    return EXIT_SUCCESS;
}
