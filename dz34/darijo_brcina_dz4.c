#if 1
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ipp.h"

#define BLOCK_WIDTH 8
#define BLOCK_HEIGHT 8
#define BLOCK_DIM (BLOCK_WIDTH * BLOCK_HEIGHT)

typedef struct {
	char type[3];
	short width, height, maxValue;
	Ipp8u* data;
} PPMImage;

Ipp8u blockRgb[BLOCK_DIM * 3];
Ipp8u blockYCbCr[3][BLOCK_DIM];
Ipp16s dctCoeffs[3][BLOCK_DIM];

static const Ipp16u qLum[BLOCK_DIM] = {
		16, 11, 10, 16, 24, 40, 51, 61,
		12, 12, 14, 19, 26, 58, 60, 55,
		14, 13, 16, 24, 40, 57, 69, 56,
		14, 17, 22, 29, 51, 87, 80, 62,
		18, 22, 37, 56, 68, 109, 103, 77,
		24, 35, 55, 64, 81, 104, 113, 92,
		49, 64, 78, 87, 103, 121, 120, 101,
		72, 92, 95, 98, 112, 100, 103, 99
};

static const Ipp16u qChrom[BLOCK_DIM] = {
		17, 18, 24, 47, 99, 99, 99, 99,
		18, 21, 26, 66, 99, 99, 99, 99,
		24, 26, 56, 99, 99, 99, 99, 99,
		47, 66, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99
};

void skipComments(FILE* fptr) {
	unsigned char c;
	while ((c = getc(fptr)) == '#') {
		while (getc(fptr) != '\n');
	}
	ungetc(c, fptr);
}

PPMImage readPPMImage(const char* ppmFile) {
	FILE* fptr;
	char magicNumber[3];
	short width, height, maxValue;
	int size;
	Ipp8u* data;
	int stepBytes;
	PPMImage img = { 0 };

	fptr = fopen(ppmFile, "rb");
	if (fptr == NULL) {
		fprintf(stderr, "ERROR fopen(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	skipComments(fptr);

	if (fscanf(fptr, "%s ", magicNumber) != 1) {
		fprintf(stderr, "ERROR fscanf(): magic number\n");
		exit(EXIT_FAILURE);
	}
	magicNumber[2] = '\0';

	skipComments(fptr);

	if (fscanf(fptr, "%hd %hd ", &width, &height) != 2) {
		fprintf(stderr, "ERROR fscanf(): width and height\n");
		exit(EXIT_FAILURE);
	}
	size = width * height;

	skipComments(fptr);

	if (fscanf(fptr, "%hd ", &maxValue) != 1) {
		fprintf(stderr, "ERROR fscanf(): max value\n");
		exit(EXIT_FAILURE);
	}

	data = ippiMalloc_8u_C3(width, height, &stepBytes);
	if (fread(data, 3, size, fptr) != size) {
		fprintf(stderr, "ERROR fread(): invalid data\n");
		ippiFree(data);
		exit(EXIT_FAILURE);
	}
	fclose(fptr);

	strcpy(img.type, magicNumber);
	img.width = width;
	img.height = height;
	img.maxValue = maxValue;
	img.data = data;
	return img;
}

void freePPMImage(PPMImage* img) {
	ippiFree(img->data);
}

void getBlock(PPMImage* img, int blockNumber, int xBlockCount, int yBlockCount) {
	int xOffset, yOffset, x, y, counter;
	xOffset = (blockNumber % xBlockCount) * BLOCK_WIDTH;
	yOffset = (blockNumber / yBlockCount) * BLOCK_HEIGHT * img->width * 3;
	y = yOffset;
	counter = 0;
	for (size_t i = 0; i < BLOCK_HEIGHT; i++) {
		x = xOffset;
		for (size_t j = 0; j < BLOCK_WIDTH * 3; j++) {
			blockRgb[counter++] = img->data[y + x];
			x++;
		}
		y += img->width * 3;
	}
}

void rgb2YCbCr() {
	Ipp8u* temp[3] = { 0 };
	IppiSize roiSize = { 8, 8 };
	IppStatus status;
	temp[0] = blockYCbCr[0];
	temp[1] = blockYCbCr[1];
	temp[2] = blockYCbCr[2];
	status = ippiRGBToYCbCr_8u_C3P3R(blockRgb, 24, temp, 8, roiSize);
}

void dct() {
	IppiSize roiSize = { 8, 8 };
	IppStatus status;
	status = ippiDCT8x8FwdLS_8u16s_C1R(blockYCbCr[0], 8, dctCoeffs[0], -128);
	status = ippiDCT8x8FwdLS_8u16s_C1R(blockYCbCr[1], 8, dctCoeffs[1], -128);
	status = ippiDCT8x8FwdLS_8u16s_C1R(blockYCbCr[2], 8, dctCoeffs[2], -128);
}

void quantize() {
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < BLOCK_DIM; j++) {
			Ipp16s coef = i == 0 ? qLum[j] : qChrom[j];
			dctCoeffs[i][j] /= coef;
		}
	}
}

int main(int argc, char** argv) {
	if (argc != (1 + 1)) {
		fprintf(stderr, "Program expects path to some .ppm image file!\n");
		return EXIT_FAILURE;
	}

	clock_t startTime, endTime, timeDifference;
	double timeInSeconds;
	char* ppmFile;
	PPMImage img;
	int xBlockCount, yBlockCount, nBlocks;

	startTime = clock();

	ppmFile = argv[1];
	img = readPPMImage(ppmFile);
	xBlockCount = img.width / BLOCK_WIDTH;
	yBlockCount = img.height / BLOCK_HEIGHT;
	nBlocks = xBlockCount * yBlockCount;

	for (int i = 0; i < nBlocks; i++) {
		getBlock(&img, i, xBlockCount, yBlockCount);
		rgb2YCbCr();
		dct();
		quantize();
	}

	freePPMImage(&img);

	endTime = clock();
	timeDifference = endTime - startTime;
	timeInSeconds = (double)timeDifference / CLOCKS_PER_SEC;

	fprintf(stdout, "Vrijeme izvodjenja: %f s.\n", timeInSeconds);
	return EXIT_SUCCESS;
}
#endif