#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#define BLOCK_WIDTH 16
#define BLOCK_HEIGHT 16
#define BLOCK_SIZE (BLOCK_WIDTH * BLOCK_HEIGHT)

typedef struct {
    uint8_t val;
} PixelGS8;

typedef struct {
    char type[3];
    uint16_t width, height, maxVal;
    PixelGS8 **data;
} ImagePGM;

typedef struct {
    int32_t x;
    int32_t y;
    double mad;
} Point;

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
    PixelGS8 *linearData = (PixelGS8 *) malloc(sizeof(PixelGS8) * size);
    if (fread(linearData, sizeof(PixelGS8), size, file) != size) {
        fprintf(stderr, "readPGMImage()::fscanf() - Parsing data failed!\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    PixelGS8 **matrixData = (PixelGS8 **) malloc(sizeof(PixelGS8 *) * height);
    for (size_t i = 0; i < height; ++i) {
        matrixData[i] = (PixelGS8 *) malloc(sizeof(PixelGS8) * width);
        for (size_t j = 0; j < width; ++j) {
            matrixData[i][j] = linearData[i * width + j];
        }
    }
    free(linearData);

    ImagePGM image = {.width=width, .height=height, .maxVal=maxVal, .data=matrixData};
    strcpy(image.type, magicNumber);
    return image;
}

void freePGMImage(ImagePGM *img) {
    for (size_t i = 0; i < img->height; ++i) {
        free(img->data[i]);
    }
    free(img->data);
}

PixelGS8 *getPGMImageBlock(ImagePGM *img, uint32_t originX, uint32_t originY) {
    PixelGS8 *block = (PixelGS8 *) malloc(sizeof(PixelGS8) * BLOCK_SIZE);
    uint32_t counter = 0;
    for (size_t i = 0; i < BLOCK_HEIGHT; ++i) {
        PixelGS8 *row = img->data[originY + i];
        for (size_t j = 0; j < BLOCK_WIDTH; ++j) {
            block[counter++] = row[originX + j];
        }
    }
    return block;
}

double calculateMAD(PixelGS8 *currentImgBlock, PixelGS8 *previousImgBlock) {
    double result = 0.0;
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        result += abs(currentImgBlock[i].val - previousImgBlock[i].val);
    }
    return result / BLOCK_SIZE;
}

Point findMovementVector(ImagePGM *currentImg, ImagePGM *previousImg, uint16_t blockIndex) {
    Point vector;

    uint16_t width = currentImg->width;
    uint16_t height = currentImg->height;

    uint32_t xBlockCount = width / BLOCK_WIDTH;
    uint32_t yBlockCount = height / BLOCK_HEIGHT;
    uint32_t currentImgOriginX = blockIndex % xBlockCount * BLOCK_WIDTH;
    uint32_t currentImgOriginY = blockIndex / yBlockCount * BLOCK_HEIGHT;
    PixelGS8 *currentImgBlock = getPGMImageBlock(currentImg, currentImgOriginX, currentImgOriginY);

    int32_t previousImgOriginY = (int32_t) (currentImgOriginY - BLOCK_HEIGHT);
    int32_t previousImgEndY = (int32_t) (currentImgOriginY + BLOCK_HEIGHT);
    int32_t previousImgOriginX = (int32_t) (currentImgOriginX - BLOCK_WIDTH);
    int32_t previousImgEndX = (int32_t) (currentImgOriginX + BLOCK_WIDTH);

    double minMAD = DBL_MAX;
    for (int32_t originY = previousImgOriginY; originY <= previousImgEndY; ++originY) {
        if (originY < 0) continue;
        if (originY + BLOCK_HEIGHT - 1 >= height) break;
        for (int32_t originX = previousImgOriginX; originX <= previousImgEndX; ++originX) {
            if (originX < 0) continue;
            if (originX + BLOCK_WIDTH - 1 >= width) break;
            PixelGS8 *previousImgBlock = getPGMImageBlock(previousImg, originX, originY);
            double currentMAD = calculateMAD(currentImgBlock, previousImgBlock);
            free(previousImgBlock);
            if (currentMAD < minMAD) {
                minMAD = currentMAD;
                vector.x = originX - currentImgOriginX;
                vector.y = originY - currentImgOriginY;
                vector.mad = minMAD;
            }
        }
    }

    free(currentImgBlock);
    return vector;
}

int main(int argc, char *argv[]) {
    uint16_t blockIndex = atoi(argv[1]);
    ImagePGM currentImg;
    ImagePGM previousImg;
    if (argc < 3) {
        currentImg = readPGMImage("lenna1.pgm");
        previousImg = readPGMImage("lenna.pgm");
    } else {
        currentImg = readPGMImage(argv[2]);
        previousImg = readPGMImage(argv[3]);
    }

    Point vector = findMovementVector(&currentImg, &previousImg, blockIndex);
    fprintf(stdout, "%d,%d\n", vector.x, vector.y);

    freePGMImage(&currentImg);
    freePGMImage(&previousImg);
    return EXIT_SUCCESS;
}
