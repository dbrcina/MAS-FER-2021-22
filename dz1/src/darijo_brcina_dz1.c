#include <stdio.h>
#include <stdlib.h>

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

#define DCT_BLOCK_SIZE 8
#define TRANSLATE_CONST 128

typedef struct {
    unsigned char r, g, b;
} PixelRGB;

typedef struct {
    float y, cb, cr;
} PixelYCbCr;

typedef struct {
    char *type;
    unsigned short width, height, maxValue;
    PixelRGB *pixels;
} PPMImageRGB;

typedef struct {
    unsigned short width, height;
    PixelYCbCr *pixels;
} PPMImageYCbCr;

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

PixelRGB *retrieveBlock(const unsigned short blockNumber) {

}

//PixelYCbCr *fromRGBToYCbCr(PPMImageRGB *imageRGB, const unsigned short blockNumber) {
//    PixelYCbCr *pixelsYCbCr = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * DCT_BLOCK_SIZE * DCT_BLOCK_SIZE);
//    unsigned short width = imageRGB->width;
//    unsigned short height = imageRGB->height;
//    int size = width * height;
//    PixelRGB *pixelsRGB = imageRGB->pixels;
//    for (int i = 0; i < size; ++i) {
//        PixelRGB pixelRGB = pixelsRGB[i];
//        float y = Y_R_CONST * pixelRGB.r + Y_G_CONST * pixelRGB.g + Y_B_CONST * pixelRGB.b;
//        float cb = Cb_R_CONST * pixelRGB.r + Cb_G_CONST * pixelRGB.g + Cb_B_CONST * pixelRGB.b + Cb_ADD_CONST;
//        float cr = Cr_R_CONST * pixelRGB.r + Cr_G_CONST * pixelRGB.g + Cr_B_CONST * pixelRGB.b + Cr_ADD_CONST;
//        pixelsYCbCr[i] = (PixelYCbCr) {.y=y, .cb=cb, .cr=cr};
//    }
//    return (PPMImageYCbCr) {.width = width, .height=height, .pixels=pixelsYCbCr};
//}
//
//PPMImageYCbCr fromRGBToYCbCr(PPMImageRGB *imageRGB) {
//    unsigned short width = imageRGB->width;
//    unsigned short height = imageRGB->height;
//    int size = width * height;
//    PixelRGB *pixelsRGB = imageRGB->pixels;
//    PixelYCbCr *pixelsYCbCr = (PixelYCbCr *) malloc(sizeof(PixelYCbCr) * size);
//    for (int i = 0; i < size; ++i) {
//        PixelRGB pixelRGB = pixelsRGB[i];
//        float y = Y_R_CONST * pixelRGB.r + Y_G_CONST * pixelRGB.g + Y_B_CONST * pixelRGB.b;
//        float cb = Cb_R_CONST * pixelRGB.r + Cb_G_CONST * pixelRGB.g + Cb_B_CONST * pixelRGB.b + Cb_ADD_CONST;
//        float cr = Cr_R_CONST * pixelRGB.r + Cr_G_CONST * pixelRGB.g + Cr_B_CONST * pixelRGB.b + Cr_ADD_CONST;
//        pixelsYCbCr[i] = (PixelYCbCr) {.y=y, .cb=cb, .cr=cr};
//    }
//    return (PPMImageYCbCr) {.width = width, .height=height, .pixels=pixelsYCbCr};
//}

void translateYCbCrImage(PPMImageYCbCr *image) {

}

int main(int argc, const char *argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr, "Program expects path to some .ppm image file, block number and output file!\n");
        return EXIT_FAILURE;
    }
    const char *inFile = argv[1];
    const unsigned short blockNumber = atoi(argv[2]);
    const char *outFile = argv[3];

    int *data = (int *) malloc(4 * 16);
    int *block = (int *) malloc(4 * 4);

    for (int i = 0; i < 16; ++i) {
        data[i] = i;
    }
    int counter = 0;
    int blockSize = 2;
    int width = 4;
    int blockN = 2;
    int yOffset = blockNumber / blockN * blockSize * width;
    int xOffset = blockNumber % blockN * blockSize;
    for (int i = 0, y = yOffset; i < blockSize; ++i, y += width) {
        for (int j = 0, x = xOffset; j < blockSize; ++j, ++x) {
            block[counter++] = data[y + x];
        }
    }

    printf("%d %d\n", block[0], block[1]);
    printf("%d %d\n", block[2], block[3]);

    free(data);
    free(block);

    // Load image
//    PPMImageRGB imageRGB = parsePPMImageRGB(inFile);
//    // Transform from RGB to YCbCr
//    PPMImageYCbCr imageYCbCr = fromRGBToYCbCr(&imageRGB);
//
//    free(imageRGB.pixels);
//    free(imageYCbCr.pixels);
    return EXIT_SUCCESS;
}
