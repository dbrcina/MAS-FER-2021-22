#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
    if (argc != (1 + 3)) {
        fprintf(stderr,
                "Program expects path to some .ppm image file, block number and output file for storing the results!\n");
        return EXIT_FAILURE;
    }

    FILE *fptr = fopen(argv[1], "r");
    if (fptr == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    fclose(fptr);
    return EXIT_SUCCESS;
}
