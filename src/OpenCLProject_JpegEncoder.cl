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
