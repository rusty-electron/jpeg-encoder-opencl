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

__kernel void quantizationKernel(__global float* d_input, __global float* d_output, __global uint* quant_lum, __global uint* quant_chrom, const unsigned int width, const unsigned int height) {
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

    d_output[pixel_index] = y / (float)quant_lum[pixel_index % 64];
    d_output[width * height + pixel_index] = u / (float)quant_chrom[pixel_index % 64];
    d_output[2 * width * height + pixel_index] = v / (float)quant_chrom[pixel_index % 64];
}