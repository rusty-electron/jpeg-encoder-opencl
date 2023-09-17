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

void copyToLargerImage(ppm_t *, ppm_t *);
void getNearest8x8ImageSize(size_t, size_t, size_t *, size_t *);

void previewImage(ppm_t *, size_t, size_t, size_t, size_t);