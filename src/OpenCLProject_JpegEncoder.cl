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

__kernel void chromaSubsamplingKernel(__global uint* d_input, __global uint* d_output, const unsigned int width, const unsigned int height) {
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);

    size_t countX = get_global_size(0);
    size_t countY = get_global_size(1);

    // TODO: optimize this to fully utilize all workers

    size_t eff_i = i * 2;
    size_t eff_j = j * 2;

    if (eff_i >= width || eff_j >= height) {
        return;
    }

    size_t pixel_1_index = eff_j * width + eff_i;
    size_t pixel_2_index = eff_j * width + eff_i + 1;
    size_t pixel_3_index = (eff_j + 1) * width + eff_i;
    size_t pixel_4_index = (eff_j + 1) * width + eff_i + 1;

    uint cb1 = d_input[pixel_1_index + width * height];
    uint cb2 = d_input[pixel_2_index + width * height];
    uint cb3 = d_input[pixel_3_index + width * height];
    uint cb4 = d_input[pixel_4_index + width * height];

    uint cr1 = d_input[pixel_1_index + 2 * width * height];
    uint cr2 = d_input[pixel_2_index + 2 * width * height];
    uint cr3 = d_input[pixel_3_index + 2 * width * height];
    uint cr4 = d_input[pixel_4_index + 2 * width * height];

    uint cb = (uint)((cb1 + cb2 + cb3 + cb4) / 4.0);
    uint cr = (uint)((cr1 + cr2 + cr3 + cr4) / 4.0);
    // uint cb = 1;
    // uint cr = 2;

    d_output[pixel_1_index + width * height] = cb;
    d_output[pixel_2_index + width * height] = cb;
    d_output[pixel_3_index + width * height] = cb;
    d_output[pixel_4_index + width * height] = cb;

    d_output[pixel_1_index + 2 * width * height] = cr;
    d_output[pixel_2_index + 2 * width * height] = cr;
    d_output[pixel_3_index + 2 * width * height] = cr;
    d_output[pixel_4_index + 2 * width * height] = cr;

    // almost forgot the y values
    d_output[pixel_1_index] = d_input[pixel_1_index];
    d_output[pixel_2_index] = d_input[pixel_2_index];
    d_output[pixel_3_index] = d_input[pixel_3_index];
    d_output[pixel_4_index] = d_input[pixel_4_index];
}

__kernel void quantizationKernel(__global float* d_input, __global int* d_output, __global uint* quant_lum, __global uint* quant_chrom, const unsigned int width, const unsigned int height) {
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);

    size_t countX = get_global_size(0);
    size_t countY = get_global_size(1);

    if (i >= width || j >= height) {
        return;
    }

    size_t pixel_index = j * width + i;

    float y = d_input[pixel_index];
    float u = d_input[width * height + pixel_index];
    float v = d_input[2 * width * height + pixel_index];

    d_output[pixel_index] = round(y / (float)quant_lum[pixel_index % 64]);
    d_output[width * height + pixel_index] = round(u / (float)quant_chrom[pixel_index % 64]);
    d_output[2 * width * height + pixel_index] = round(v / (float)quant_chrom[pixel_index % 64]);
}

__kernel void zigzagKernel2(__global int* d_input, __global int* d_output) {
    size_t i = get_global_id(0); // MCU index
    
    size_t count = get_global_size(0);

    if (i >= count) {
        return;
    }

    int zigzag[64] = { 0, 1, 8, 16, 9, 2, 3, 10,
                       17, 24, 32, 25, 18, 11, 4, 5,
                       12, 19, 26, 33, 40, 48, 41, 34,
                       27, 20, 13, 6, 7, 14, 21, 28,
                       35, 42, 49, 56, 57, 50, 43, 36,
                       29, 22, 15, 23, 30, 37, 44, 51,
                       58, 59, 52, 45, 38, 31, 39, 46,
                       53, 60, 61, 54, 47, 55, 62, 63 };

    for (int j = 0; j < 64; j++) {
        d_output[i * 64 + j] = d_input[i * 64 + zigzag[j]];
    }
}

__kernel void zigzagKernel(__global int* d_input, __global int* d_output) {
    size_t i = get_global_id(0); // MCU index
    size_t j = get_global_id(1); // index within MCU
    
    size_t count_i = get_global_size(0);
    size_t count_j = get_global_size(1);


    if (i >= count_i || j >= count_j) {
        return;
    }

    int zigzag[64] = { 0, 1, 8, 16, 9, 2, 3, 10,
                       17, 24, 32, 25, 18, 11, 4, 5,
                       12, 19, 26, 33, 40, 48, 41, 34,
                       27, 20, 13, 6, 7, 14, 21, 28,
                       35, 42, 49, 56, 57, 50, 43, 36,
                       29, 22, 15, 23, 30, 37, 44, 51,
                       58, 59, 52, 45, 38, 31, 39, 46,
                       53, 60, 61, 54, 47, 55, 62, 63 };

    d_output[i * 64 + j] = d_input[i * 64 + zigzag[j]];
}

__kernel void rleKernel(__global int* d_input, __global int* d_output) {
    size_t i = get_global_id(0); // MCU index
    
    size_t count = get_global_size(0);

    if (i >= count) {
        return;
    }

    int count_zeroes = 0;
    int last_non_zero_index = 0;

    for (int k = 63; k >= 0; k--) {
        if (d_input[i * 64 + k] != 0) {
            last_non_zero_index = k;
            break;
        }
    }

    int j;
    for (j = 0; j <= last_non_zero_index; j++) {
        if (d_input[i * 64 + j] == 0) {
            if (count_zeroes == 15) {
                d_output[i * 64 + 2 * j] = count_zeroes;
                d_output[i * 64 + 2 * j + 1] = 0;
                count_zeroes = 0;
            } else {
                count_zeroes++;
            }
        } else {
            d_output[i * 64 + 2 * j] = count_zeroes;
            d_output[i * 64 + 2 * j + 1] = d_input[i * 64 + j];
            count_zeroes = 0;
        }
    }
    d_output[i * 64 + 2 * j] = 0;
    d_output[i * 64 + 2 * j + 1] = 0;
}