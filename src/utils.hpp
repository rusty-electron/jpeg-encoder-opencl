#pragma once
#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <cstring>

struct rgb_pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct rgb_pixel rgb_pixel_t;

struct PPMimage {
    size_t width;
    size_t height;
    rgb_pixel_t *data;
};

typedef struct PPMimage ppm_t;

// data structure for intermediate steps

struct rgb_pixel_d {
    double r;
    double g;
    double b;
};

typedef struct rgb_pixel_d rgb_pixel_d_t;

struct PPMimage_d {
    size_t width;
    size_t height;
    rgb_pixel_d_t *data;
};

typedef struct PPMimage_d ppm_d_t;

// for 50% quality
const unsigned int quant_mat_lum[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

const unsigned int quant_mat_chrom[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
};

int readPPMImage(const char *, size_t *, size_t *, rgb_pixel_t **);
int writePPMImage(const char *, size_t, size_t, rgb_pixel_t *);
void removeRedChannel(ppm_t *);

void performCSC(ppm_t *);
void performCDS(ppm_t *);

rgb_pixel_t* getPixelPtr(ppm_t *, size_t, size_t);
rgb_pixel_t getPixel(ppm_t *, size_t, size_t);
uint8_t getPixelR(ppm_t *, size_t, size_t);
uint8_t getPixelG(ppm_t *, size_t, size_t);
uint8_t getPixelB(ppm_t *, size_t, size_t);

void setPixelR(ppm_t *, size_t, size_t, uint8_t);
void setPixelG(ppm_t *, size_t, size_t, uint8_t);
void setPixelB(ppm_t *, size_t, size_t, uint8_t);

void copyUIntToDoubleImage(ppm_t *, ppm_d_t *);
void copyDoubleToUIntImage(ppm_d_t *, ppm_t *);

void copyToLargerImage(ppm_t *, ppm_t *);
void getNearest8x8ImageSize(size_t, size_t, size_t *, size_t *);
void addReversedPadding(ppm_t *, size_t, size_t);
void substractfromAll(ppm_d_t *, double);

void performDCT(ppm_d_t *);
void performDCTBlock(ppm_d_t *, size_t, size_t);

void performQuantization(ppm_d_t *, const unsigned int[][8], const unsigned int[][8]);

void previewImage(ppm_t *, size_t, size_t, size_t, size_t);
void previewImageD(ppm_d_t *, size_t, size_t, size_t, size_t);

void previewImage(ppm_t *, size_t, size_t, size_t, size_t);

//void testQuantize(ppm_t *, double);

void diagonalZigZag(ppm_d_t * ,float *);

void seperateChannels(ppm_d_t * ,float * , float * , float * , float * );
void RLE(float *, float * , int );
