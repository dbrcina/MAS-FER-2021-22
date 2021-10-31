#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#define BLOCK_DIM 8
#define BLOCK_SIZE BLOCK_DIM * BLOCK_DIM

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

float const k1Table[BLOCK_SIZE] = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99
};

float const k2Table[BLOCK_SIZE] = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
};

typedef struct {
    uint8_t r, g, b;
} PixelRGB;

typedef struct {
    float y, cb, cr;
} PixelYCbCr;

typedef struct {
    int8_t y, cb, cr;
} PixelYCbCrQuantized;

typedef struct {
    char *type;
    uint16_t width, height, maxValue;
    PixelRGB *pixels;
} PPMImageRGB;

void skipComments(FILE *const fptr) {
    unsigned char c;
    while ((c = getc(fptr)) == '#') {
        while (getc(fptr) != '\n');
    }
    ungetc(c, fptr);
}

PPMImageRGB parsePPMImageRGB(const char *const file) {
    FILE *const fptr = fopen(file, "rb");
    if (fptr == NULL) {
        perror("parsePPMImageRGB::fopen()");
        exit(EXIT_FAILURE);
    }

    // Parse "magic number"
    char magicNumber[3];
    if (fscanf(fptr, "%s ", magicNumber) != 1) {
        perror("parsePPMImageRGB::magic number fscanf()");
        exit(EXIT_FAILURE);
    }

    // Skip all possible comments
    skipComments(fptr);

    // Parse width and height
    uint16_t width, height;
    if (fscanf(fptr, "%hu %hu ", &width, &height) != 2) {
        perror("parsePPMImageRGB::width and height fscanf()");
        exit(EXIT_FAILURE);
    }

    // Skip all possible comments
    skipComments(fptr);

    // Parse max RGB value
    uint16_t maxValue;
    if (fscanf(fptr, "%hu ", &maxValue) != 1) {
        perror("parsePPMImageRGB::max value fscanf()");
        exit(EXIT_FAILURE);
    }

    // Skip all possible comments
    skipComments(fptr);

    // Parse data
    uint32_t const size = width * height;
    PixelRGB *const pixels = (PixelRGB *) malloc(sizeof(PixelRGB) * size);
    if (fread(pixels, sizeof(PixelRGB), size, fptr) != size) {
        perror("parsePPMImageRGB::fread()");
        exit(EXIT_FAILURE);
    }

    fclose(fptr);
    return (PPMImageRGB) {.type=magicNumber, .width=width, .height=height, .maxValue=maxValue, .pixels=pixels};
}

PixelRGB *retrieveRGBBlock(const PPMImageRGB *const imageRGB, uint32_t const blockNumber) {
    const PixelRGB *const pixels = imageRGB->pixels;
    PixelRGB *const blockRGB = (PixelRGB *) malloc(sizeof(PixelRGB) * BLOCK_SIZE);
    uint32_t const xBlockCount = imageRGB->width / BLOCK_DIM;
    uint32_t const yBlockCount = imageRGB->height / BLOCK_DIM;
    uint32_t const yOffset = blockNumber / yBlockCount * BLOCK_DIM * imageRGB->width;
    uint32_t const xOffset = blockNumber % xBlockCount * BLOCK_DIM;
    for (size_t i = 0, y = yOffset, counter = 0; i < BLOCK_DIM; ++i, y += imageRGB->width) {
        for (size_t j = 0, x = xOffset; j < BLOCK_DIM; ++j, ++x) {
            blockRGB[counter++] = pixels[y + x];
        }
    }
    return blockRGB;
}

PixelYCbCr *fromRGBToYCbCr(const PixelRGB *const blockRGB) {
    PixelYCbCr *const blockYCbCr = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * BLOCK_SIZE);
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        PixelRGB const pixelRGB = blockRGB[i];
        float const y = Y_R_CONST * pixelRGB.r + Y_G_CONST * pixelRGB.g + Y_B_CONST * pixelRGB.b;
        float const cb = Cb_R_CONST * pixelRGB.r + Cb_G_CONST * pixelRGB.g + Cb_B_CONST * pixelRGB.b + Cb_ADD_CONST;
        float const cr = Cr_R_CONST * pixelRGB.r + Cr_G_CONST * pixelRGB.g + Cr_B_CONST * pixelRGB.b + Cr_ADD_CONST;
        blockYCbCr[i] = (PixelYCbCr) {.y=y, .cb=cb, .cr=cr};
    }
    return blockYCbCr;
}

void shiftBlockYCbCr(PixelYCbCr *const blockYCbCr) {
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        PixelYCbCr *const pixel = blockYCbCr + i;
        pixel->y -= SHIFT_CONST;
        pixel->cb -= SHIFT_CONST;
        pixel->cr -= SHIFT_CONST;
    }
}

PixelYCbCr *dctOnBlockYCbCr(PixelYCbCr *const blockYCbCr) {
    float const cSqrt = 1 / (float) sqrt(2);
    PixelYCbCr *const dctBlock = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * BLOCK_SIZE);
    for (size_t u = 0; u < BLOCK_DIM; ++u) {
        float const cu = u == 0 ? cSqrt : 1;
        for (size_t v = 0; v < BLOCK_DIM; ++v) {
            float const cv = v == 0 ? cSqrt : 1;
            float tmpY = 0, tmpCb = 0, tmpCr = 0;
            for (size_t i = 0; i < BLOCK_DIM; ++i) {
                for (size_t j = 0; j < BLOCK_DIM; ++j) {
                    float const common = (float) (cos((2 * i + 1) * u * M_PI / 16) * cos((2 * j + 1) * v * M_PI / 16));
                    PixelYCbCr const block = blockYCbCr[i * BLOCK_DIM + j];
                    tmpY += block.y * common;
                    tmpCb += block.cb * common;
                    tmpCr += block.cr * common;
                }
            }
            PixelYCbCr *const block = dctBlock + u * BLOCK_DIM + v;
            block->y = 0.25f * cu * cv * tmpY;
            block->cb = 0.25f * cu * cv * tmpCb;
            block->cr = 0.25f * cu * cv * tmpCr;
        }
    }
    return dctBlock;
}

PixelYCbCrQuantized *quantizeBlock(const PixelYCbCr *const block) {
    PixelYCbCrQuantized *const quantizedBlock = (PixelYCbCrQuantized *)
            malloc(sizeof(PixelYCbCrQuantized) * BLOCK_SIZE);
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        PixelYCbCr const pixel = block[i];
        quantizedBlock[i] = (PixelYCbCrQuantized) {
                .y = (int8_t) round((double) pixel.y / (double) k1Table[i]),
                .cb = (int8_t) round((double) pixel.cb / (double) k2Table[i]),
                .cr = (int8_t) round((double) pixel.cr / (double) k2Table[i])};
    }
    return quantizedBlock;
}

void writeToFile(const PixelYCbCrQuantized *const quantizedPixels, const char *const file) {
    FILE *const fptr = fopen(file, "w");
    if (fptr == NULL) {
        perror("writeToFile::fopen()");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < BLOCK_DIM; ++i) {
        for (size_t j = 0; j < BLOCK_DIM; ++j) {
            fprintf(fptr, "%d%s", quantizedPixels[i * BLOCK_DIM + j].y, j != BLOCK_DIM - 1 ? " " : "");
        }
        fprintf(fptr, "\n");
    }
    fprintf(fptr, "\n");
    for (size_t i = 0; i < BLOCK_DIM; ++i) {
        for (size_t j = 0; j < BLOCK_DIM; ++j) {
            fprintf(fptr, "%d%s", quantizedPixels[i * BLOCK_DIM + j].cb, j != BLOCK_DIM - 1 ? " " : "");
        }
        fprintf(fptr, "\n");
    }
    fprintf(fptr, "\n");
    for (size_t i = 0; i < BLOCK_DIM; ++i) {
        for (size_t j = 0; j < BLOCK_DIM; ++j) {
            fprintf(fptr, "%d%s", quantizedPixels[i * BLOCK_DIM + j].cr, j != BLOCK_DIM - 1 ? " " : "");
        }
        fprintf(fptr, "\n");
    }
    fclose(fptr);
}

int main(int32_t const argc, const char *const argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr, "Program expects path to some .ppm image file, block number and output file!\n");
        return EXIT_FAILURE;
    }

    const char *const inFile = argv[1];
    uint32_t const blockNumber = atoi(argv[2]);
    const char *const outFile = argv[3];

    // Load image
    PPMImageRGB const imageRGB = parsePPMImageRGB(inFile);

    // Retrieve RGB block
    PixelRGB *const blockRGB = retrieveRGBBlock(&imageRGB, blockNumber);
    // Free not needed memory...
    free(imageRGB.pixels);

    // Transform from RGB to YCbCr
    PixelYCbCr *const blockYCbCr = fromRGBToYCbCr(blockRGB);
    // Free not needed memory...
    free(blockRGB);

    // Shift pixels by 128
    shiftBlockYCbCr(blockYCbCr);

    // Apply DCT
    PixelYCbCr *const dctBlock = dctOnBlockYCbCr(blockYCbCr);
    // Free not needed memory...
    free(blockYCbCr);

    // Apply quantization
    PixelYCbCrQuantized *const quantizedPixels = quantizeBlock(dctBlock);
    // Free not needed memory...
    free(dctBlock);

    writeToFile(quantizedPixels, outFile);

    free(quantizedPixels);
    return EXIT_SUCCESS;
}
