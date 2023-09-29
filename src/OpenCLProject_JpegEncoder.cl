#ifndef __OPENCL_VERSION__
#include <OpenCL/OpenCLKernel.hpp> // Hack to make syntax highlighting in Eclipse work
#endif

__kernel void colorConversionKernel(__global uint* d_input, __global uint* d_output, const unsigned int width, const unsigned int height) {
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);

    size_t countX = get_global_size(0);
    size_t countY = get_global_size(1);

    if (i >= width || j >= height) {
        return;
    }

    // get pixel values
    uint red_pixel = d_input[j * width + i];
    uint green_pixel = d_input[width * height + j * width + i];
    uint blue_pixel = d_input[2 * width * height + j * width + i];

    uint y = (uint)(0.299f * red_pixel + 0.587f * green_pixel + 0.114f * blue_pixel);
    uint u = (uint)(-0.169f * red_pixel - 0.331f * green_pixel + 0.500f * blue_pixel + 128);
    uint v = (uint)(0.500f * red_pixel - 0.419f * green_pixel - 0.081f * blue_pixel + 128);

    d_output[j * width + i] = y;
    d_output[width * height + j * width + i] = u;
    d_output[2 * width * height + j * width + i] = v;
}

__kernel void performDCTBlock(__global ppm_d_t* img, int startX, int startY) {
    int u = get_global_id(0);  // Thread ID along the x-axis
    int v = get_global_id(1);  // Thread ID along the y-axis

    int width = img->width;
    int height = img->height;

    double alphaU = (u == 0) ? 1.0 : 1.0 / sqrt(2.0);
    double alphaV = (v == 0) ? 1.0 : 1.0 / sqrt(2.0);

    double sumR = 0.0;
    double sumG = 0.0;
    double sumB = 0.0;

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            int pixelX = startX + x;
            int pixelY = startY + y;

            if (pixelX < width && pixelY < height) {
                __global rgb_pixel_d_t* pixel = &img->data[pixelY * width + pixelX];
                double cosValX = cos((2.0 * pixelX + 1.0) * u * M_PI / 16.0);
                double cosValY = cos((2.0 * pixelY + 1.0) * v * M_PI / 16.0);

                sumR += pixel->r * cosValX * cosValY;
                sumG += pixel->g * cosValX * cosValY;
                sumB += pixel->b * cosValX * cosValY;
            }
        }
    }

    sumR *= (alphaU * alphaV / 4.0);
    sumG *= (alphaU * alphaV / 4.0);
    sumB *= (alphaU * alphaV / 4.0);

    int x_val = startX + u;
    int y_val = startY + v;
    
    if (x_val < width && y_val < height) {
        __global rgb_pixel_d_t* pixel = &img->data[y_val * width + x_val];
        pixel->r = sumR;
        pixel->g = sumG;
        pixel->b = sumB;
    }
}

__kernel void performDCT(__global ppm_d_t* img) {
    int startX = get_global_id(0) * 8;
    int startY = get_global_id(1) * 8;

    performDCTBlock(img, startX, startY);
}
