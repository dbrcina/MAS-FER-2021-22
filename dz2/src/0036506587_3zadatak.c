#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define N_GROUPS 16

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


int main(int argc, char *argv[]) {
    ImagePGM image = readPGMImage(argc < 2 ? "lenna.pgm" : argv[1]);
    uint32_t groups[N_GROUPS] = {0};
    uint32_t size = image.width * image.height;
    for (size_t i = 0; i < size; ++i) {
        ++groups[(image.data[i].val >> 4) % N_GROUPS];
    }
    for (size_t i = 0; i < N_GROUPS; ++i) {
        fprintf(stdout, "%ld %f\n", i, (double) groups[i] / size);
    }
    free(image.data);
    return EXIT_SUCCESS;
}
