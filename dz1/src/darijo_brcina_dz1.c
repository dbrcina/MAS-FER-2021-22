#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DCT_BLOCK_SIZE 8

#define Y_R_CONST 0.299
#define Y_G_CONST 0.587
#define Y_B_CONST 0.114

#define Cb_R_CONST (-0.1687)
#define Cb_G_CONST (-0.3313)
#define Cb_B_CONST 0.5
#define Cb_ADD_CONST 128

#define Cr_R_CONST 0.5
#define Cr_G_CONST (-0.4187)
#define Cr_B_CONST (-0.0813)
#define Cr_ADD_CONST 128

#define SHIFT_CONST 128

#define C(u) (((u) == (0)) ? (1 / sqrt(2)) : (1))

#define M_PI 3.14159265358979323846

static const unsigned char k1Table[DCT_BLOCK_SIZE][DCT_BLOCK_SIZE] = {
        {16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 57, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}
};

static const unsigned char k2Table[DCT_BLOCK_SIZE][DCT_BLOCK_SIZE] = {
        {17, 18, 24, 47, 99, 99,  99,  99},
        {18, 21, 26, 66, 99, 99,  99,  99},
        {24, 26, 56, 99, 99, 99,  99,  99},
        {47, 66, 99, 99, 99, 99,  99,  99},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {99, 99, 99, 99, 99, 99,  99,  99},
        {99, 99, 99, 99, 99, 99,  99,  99},
        {99, 99, 99, 99, 99, 99,  99,  99}
};

typedef struct {
    unsigned char r, g, b;
} PixelRGB;

typedef struct {
    float y, cb, cr;
} PixelYCbCr;

typedef struct {
    unsigned char coeff;
} QuantizedPixel;

typedef struct {
    char *type;
    unsigned short width, height, maxValue;
    PixelRGB *pixels;
} PPMImageRGB;

PPMImageRGB parsePPMImageRGB(const char *file) {
    FILE *fptr = fopen(file, "rb");
    if (fptr == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Parse "magic number"
    char magicNumber[3];
    if (fscanf(fptr, "%s ", magicNumber) != 1) {
        perror("magic number fscanf");
        exit(EXIT_FAILURE);
    }

    // Skip all possible comments
    unsigned char c;
    while ((c = getc(fptr)) == '#') {
        while (getc(fptr) != '\n');
    }
    ungetc(c, fptr);

    // Parse width and height
    unsigned int width, height;
    if (fscanf(fptr, "%d %d ", &width, &height) != 2) {
        perror("width and height fscanf");
        exit(EXIT_FAILURE);
    } else if (width <= 0 || height <= 0) {
        fprintf(stderr, "Image size is invalid!\n");
        exit(EXIT_FAILURE);
    }

    // Parse max RGB value
    unsigned int maxValue;
    if (fscanf(fptr, "%d ", &maxValue) != 1) {
        perror("max value fscanf");
        exit(EXIT_FAILURE);
    }

    // Parse data
    PixelRGB *pixels = (PixelRGB *) malloc(sizeof(PixelRGB) * width * height);
    fread(pixels, sizeof(PixelRGB), width * height, fptr);

    fclose(fptr);
    return (PPMImageRGB) {.type=magicNumber, .width=width, .height=height, .maxValue=maxValue, .pixels=pixels};
}

PixelRGB *retrieveBlock(PPMImageRGB *image, const unsigned short blockNumber) {
    PixelRGB *pixels = image->pixels;
    PixelRGB *block = (PixelRGB *) malloc(sizeof(PixelRGB) * DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
    const unsigned short xBlockCount = image->width / DCT_BLOCK_SIZE;
    const unsigned short yBlockCount = image->height / DCT_BLOCK_SIZE;
    const int yOffset = blockNumber / yBlockCount * DCT_BLOCK_SIZE * image->width;
    const int xOffset = blockNumber % xBlockCount * DCT_BLOCK_SIZE;
    for (int i = 0, y = yOffset, counter = 0; i < DCT_BLOCK_SIZE; ++i, y += image->width) {
        for (int j = 0, x = xOffset; j < DCT_BLOCK_SIZE; ++j, ++x) {
            block[counter++] = pixels[y + x];
        }
    }
    return block;
}

PixelYCbCr *fromRGBToYCbCr(PixelRGB *blockRGB) {
    PixelYCbCr *blockYCbCr = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
    for (int i = 0; i < DCT_BLOCK_SIZE * DCT_BLOCK_SIZE; ++i) {
        PixelRGB pixelRGB = blockRGB[i];
        float y = Y_R_CONST * pixelRGB.r + Y_G_CONST * pixelRGB.g + Y_B_CONST * pixelRGB.b;
        float cb = Cb_R_CONST * pixelRGB.r + Cb_G_CONST * pixelRGB.g + Cb_B_CONST * pixelRGB.b + Cb_ADD_CONST;
        float cr = Cr_R_CONST * pixelRGB.r + Cr_G_CONST * pixelRGB.g + Cr_B_CONST * pixelRGB.b + Cr_ADD_CONST;
        blockYCbCr[i] = (PixelYCbCr) {.y=y, .cb=cb, .cr=cr};
    }
    return blockYCbCr;
}

void shiftBlockYCbCr(PixelYCbCr *blockYCbCr) {
    for (int i = 0; i < DCT_BLOCK_SIZE * DCT_BLOCK_SIZE; ++i) {
        PixelYCbCr *pixel = blockYCbCr + i;
        pixel->y -= SHIFT_CONST;
        pixel->cb -= SHIFT_CONST;
        pixel->cr -= SHIFT_CONST;
    }
}

void dctOnBlockYCbCr(PixelYCbCr *blockYCbCr) {
    PixelYCbCr *copiedBlock = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
    memcpy(copiedBlock, blockYCbCr, DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
    for (int u = 0; u < DCT_BLOCK_SIZE; ++u) {
        for (int v = 0; v < DCT_BLOCK_SIZE; ++v) {
            float tmpY = 0, tmpCb = 0, tmpCr = 0;
            for (int i = 0; i < DCT_BLOCK_SIZE; ++i) {
                for (int j = 0; j < DCT_BLOCK_SIZE; ++j) {
                    PixelYCbCr block = copiedBlock[i * DCT_BLOCK_SIZE + j];
                    tmpY += block.y * cos((2 * i + 1) * u * M_PI / 16) * cos((2 * j + 1) * v * M_PI / 16);
                    tmpCb += block.cb * cos((2 * i + 1) * u * M_PI / 16) * cos((2 * j + 1) * v * M_PI / 16);
                    tmpCr += block.cr * cos((2 * i + 1) * u * M_PI / 16) * cos((2 * j + 1) * v * M_PI / 16);
                }
            }
            float cu = C(u);
            float cv = C(v);
            PixelYCbCr block = blockYCbCr[u * DCT_BLOCK_SIZE + v];
            block.y = 0.25f * cu * cv * tmpY;
            block.cb = 0.25f * cu * cv * tmpCb;
            block.cr = 0.25f * cu * cv * tmpCr;
        }
    }
}

QuantizedPixel *quantizeBlock(PixelYCbCr *block) {
    QuantizedPixel *quantizedBlock = (QuantizedPixel *) malloc(
            3 * sizeof(QuantizedPixel) * DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
    for (int i = 0; i < DCT_BLOCK_SIZE; ++i) {
        for (int j = 0; j < DCT_BLOCK_SIZE; ++j) {
            PixelYCbCr pixel = block[i * DCT_BLOCK_SIZE + j];
            quantizedBlock[i * DCT_BLOCK_SIZE + j] = (QuantizedPixel) {.coeff=(unsigned char) round(
                    pixel.y / k1Table[i][j])};
//            quantizedBlock[i * DCT_BLOCK_SIZE + j] = (QuantizedPixel) {.coeff=(unsigned char) round(
//                    pixel.cb / k2Table[i][j])};
//            quantizedBlock[3 * DCT_BLOCK_SIZE + j] = (QuantizedPixel) {.coeff=(unsigned char) round(
//                    pixel.cr / k2Table[i][j])};
        }
    }
    return quantizedBlock;
}

int main(int argc, const char *argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr, "Program expects path to some .ppm image file, block number and output file!\n");
        return EXIT_FAILURE;
    }
    const char *inFile = argv[1];
    const unsigned short blockNumber = atoi(argv[2]);
    const char *outFile = argv[3];

    // Load image
    PPMImageRGB image = parsePPMImageRGB(inFile);

    // Retrieve block
    PixelRGB *blockRGB = retrieveBlock(&image, blockNumber);
    free(image.pixels);

    // Transform from RGB to YCbCr
    PixelYCbCr *blockYCbCr = fromRGBToYCbCr(blockRGB);
    free(blockRGB);

    // Shift pixels by 128
    shiftBlockYCbCr(blockYCbCr);

    // Apply DCT
    dctOnBlockYCbCr(blockYCbCr);

    // Apply quantization
    QuantizedPixel *quantizedPixels = quantizeBlock(blockYCbCr);

    FILE *outptr = fopen(outFile, "wb");
    for (int i = 0; i < DCT_BLOCK_SIZE; ++i) {
        for (int j = 0; j < DCT_BLOCK_SIZE; ++j) {
            fwrite(quantizedPixels + i * DCT_BLOCK_SIZE + j, sizeof(QuantizedPixel), 1, outptr);
            if (j != DCT_BLOCK_SIZE) {
                fwrite(" ", sizeof(" "), 1, outptr);
            }
        }
        fwrite("\n", sizeof("\n"), 1, outptr);
    }

    fclose(outptr);
    free(blockYCbCr);
    free(quantizedPixels);
    return EXIT_SUCCESS;
}
