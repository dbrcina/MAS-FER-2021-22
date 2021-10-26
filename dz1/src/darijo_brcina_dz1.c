#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RGB_VALUE 255

typedef struct {
    char *magicNumber;
    unsigned short width, height, maxValue;
} Header;

typedef struct {
    unsigned char r, g, b;
} Pixel;

typedef struct {
    Pixel *pixels;
} Body;

Header parseHeader(FILE *fptr) {
    // Parse "magic number"
    char magicNumber[3];
    if (fscanf(fptr, "%s ", magicNumber) != 1) {
        perror("magic number fscanf");
        exit(EXIT_FAILURE);
    } else if (strcmp(magicNumber, "P6") != 0) {
        fprintf(stderr, "Magic number should be P6!\n");
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
    } else if (maxValue != MAX_RGB_VALUE) {
        fprintf(stderr, "Max value should be %d!\n", MAX_RGB_VALUE);
        exit(EXIT_FAILURE);
    }
    return (Header) {.magicNumber = magicNumber, .width=width, .height=height, .maxValue = maxValue};
}

Body parseBody(FILE *fptr, Header header) {
    Pixel *pixels = (Pixel *) malloc(sizeof(Pixel) * header.width * header.height);
    fread(pixels, sizeof(Pixel), header.width * header.height, fptr);
    return (Body) {.pixels=pixels};
}

int main(int argc, const char *argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr, "Program expects path to some .ppm image file, block number and output file!\n");
        return EXIT_FAILURE;
    }

    FILE *fptr = fopen(argv[1], "rb");
    if (fptr == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // Parse header
    Header header = parseHeader(fptr);

    // Parse body
    Body body = parseBody(fptr, header);

    free(body.pixels);
    fclose(fptr);

    return EXIT_SUCCESS;
}
