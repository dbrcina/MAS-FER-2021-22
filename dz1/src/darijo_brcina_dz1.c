#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned char r, g, b;
} Pixel;

typedef struct {
    char *type;
    unsigned short width, height, maxValue;
    Pixel *pixels;
} PPMImage;

PPMImage parsePPMImage(const char *file) {
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
    Pixel *pixels = (Pixel *) malloc(sizeof(Pixel) * width * height);
    fread(pixels, sizeof(Pixel), width * height, fptr);
    fclose(fptr);
    return (PPMImage) {.type=magicNumber, .width=width, .height=height, .maxValue=maxValue, .pixels=pixels};
}


int main(int argc, const char *argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr, "Program expects path to some .ppm image file, block number and output file!\n");
        return EXIT_FAILURE;
    }
    const char *inFile = argv[1];
    const unsigned short blockNumber = atoi(argv[2]);
    const char *outFile = argv[3];

    PPMImage image = parsePPMImage(inFile);

    free(image.pixels);
    return EXIT_SUCCESS;
}
