//////////////////////////////////////////////////////////////////////////////
// OpenCL Project: JPEG Encoder
//////////////////////////////////////////////////////////////////////////////

// includes
#include <stdio.h>

#include <Core/Assert.hpp>
#include <Core/Time.hpp>
#include <Core/Image.hpp>
#include <OpenCL/cl-patched.hpp>
#include <OpenCL/Program.hpp>
#include <OpenCL/Event.hpp>
#include <OpenCL/Device.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

#include "utils.hpp"

//////////////////////////////////////////////////////////////////////////////
// CPU implementation
//////////////////////////////////////////////////////////////////////////////


//function for test only
void writePPM(FILE *file, uint8_t *header, uint8_t *image, int size){
    fwrite( header , 15,  1, file);//writing header information
    fwrite( image , size,  1, file);//writing image information
	fclose(file);
}

void writePPM(FILE *file, uint8_t *header, double *image, int size){
    fwrite( header , 15,  1, file);//writing header information
    fwrite( image , size,  1, file);//writing image information
	fclose(file);
}


void dct(uint8_t* image, size_t width, size_t height, uint8_t *header){
	const double sqrt2 = std::sqrt(2.0);
    const double PI = 3.14159265359;
	double *dctImg;

    dctImg = new double [width * height];

    for (size_t u = 0; u < height; ++u) {
        for (size_t v = 0; v < width; ++v) {
            double sum = 0.0;

            for (size_t x = 0; x < height; ++x) {
                for (size_t y = 0; y < width; ++y) {
                    double cu = (x == 0) ? 1.0 / sqrt2 : 1.0;
                    double cv = (y == 0) ? 1.0 / sqrt2 : 1.0;

                    size_t index = (x * width + y) * 3; // Start of YCbCr pixel
                    double yVal = static_cast<double>(image[index]); // Y channel
                    double cbVal = static_cast<double>(image[index + 1]); // Cb channel
                    double crVal = static_cast<double>(image[index + 2]); // Cr channel

                    sum += cu * cv * yVal * std::cos((2.0 * u + 1.0) * x * PI / (2.0 * height)) *
                           std::cos((2.0 * v + 1.0) * y * PI / (2.0 * width));
                }
            }

            dctImg[u * width + v] = sum;
        }
	}
	FILE *write;
	write = fopen("../data/dct.ppm","wb");
	writePPM(write, header, dctImg, (int)height*width*3);
}

void JpegEncoderHost(uint8_t *image) {
	
}

//////////////////////////////////////////////////////////////////////////////
// Main function
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	// Create a context
	//cl::Context context(CL_DEVICE_TYPE_GPU);
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.size() == 0) {
		std::cerr << "No platforms found" << std::endl;
		return 1;
	}
	int platformId = 0;
	for (size_t i = 0; i < platforms.size(); i++) {
		if (platforms[i].getInfo<CL_PLATFORM_NAME>() == "AMD Accelerated Parallel Processing") {
			platformId = i;
			break;
		}
	}
	cl_context_properties prop[4] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platforms[platformId] (), 0, 0 };
	std::cout << "Using platform '" << platforms[platformId].getInfo<CL_PLATFORM_NAME>() << "' from '" << platforms[platformId].getInfo<CL_PLATFORM_VENDOR>() << "'" << std::endl;
	cl::Context context(CL_DEVICE_TYPE_GPU, prop);

	// Get the first device of the context
	std::cout << "Context has " << context.getInfo<CL_CONTEXT_DEVICES>().size() << " devices" << std::endl;
	cl::Device device = context.getInfo<CL_CONTEXT_DEVICES>()[0];
	std::vector<cl::Device> devices;
	devices.push_back(device);
	OpenCL::printDeviceInfo(std::cout, device);

	// Create a command queue
	cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);

	// Load the source code
	cl::Program program = OpenCL::loadProgramSource(context, "../src/OpenCLProject_JpegEncoder.cl");
	// Compile the source code. This is similar to program.build(devices) but will print more detailed error messages
	OpenCL::buildProgram(program, devices);
	
	// Declare some values
	std::size_t wgSizeX = 16; // Number of work items per work group in X direction
	std::size_t wgSizeY = 16; // Number of work items per work group in Y direction
	std::size_t countX = wgSizeX * 16; // Overall number of work items in X direction = Number of elements in X direction
	std::size_t countY = wgSizeY * 16;
	countX *= 3; countY *= 3; // image channels
	std::size_t count = countX * countY; // Overall number of elements
	std::size_t size = count * sizeof (cl_uint); // Size of data in bytes

	// Allocate space for output data from CPU and GPU on the host
	std::vector<cl_uint> h_input (count);
	std::vector<cl_uint> h_outputGpu (count);

	// Allocate space for input and output data on the device
	cl::Buffer d_input = cl::Buffer(context, CL_MEM_READ_WRITE, size);
	cl::Buffer d_output = cl::Buffer(context, CL_MEM_READ_WRITE, size);

	// Initialize memory to 0xff (useful for debugging because otherwise GPU memory will contain information from last execution)
	memset(h_input.data(), 255, size);
	memset(h_outputGpu.data(), 255, size);

	queue.enqueueWriteBuffer(d_input, true, 0, size, h_input.data());
	queue.enqueueWriteBuffer(d_output, true, 0, size, h_outputGpu.data());

	// start here with your own code
    ppm_t imgCPU;
	
	if (readPPMImage("../data/fruit.ppm", &imgCPU.width, &imgCPU.height, &imgCPU.data) == -1) {
		std::cout << "Error reading the image" << std::endl;
		return 1;
	}

	// copy the image onto h_input for GPU processing
	for (size_t i = 0; i < imgCPU.width * imgCPU.height; ++i) {
		h_input[i] = imgCPU.data[i].r;
		h_input[i + imgCPU.width * imgCPU.height] = imgCPU.data[i].g;
		h_input[i + 2 * imgCPU.width * imgCPU.height] = imgCPU.data[i].b;
	}

	// Copy input data to device
	queue.enqueueWriteBuffer(d_input, true, 0, size, h_input.data(), NULL, NULL);

	// create a kernel object for color conversion
	cl::Kernel colorConversionKernel(program, "colorConversionKernel");

	// Set kernel parameters
	colorConversionKernel.setArg<cl::Buffer>(0, d_input);
	colorConversionKernel.setArg<cl::Buffer>(1, d_output);
	// convert size_t to unsigned int
	colorConversionKernel.setArg<cl_uint>(2, (cl_uint)imgCPU.width);
	colorConversionKernel.setArg<cl_uint>(3, (cl_uint)imgCPU.height);

	// Launch kernel on the compute device
	queue.enqueueNDRangeKernel(colorConversionKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, NULL);
	// Copy output data back to host
	queue.enqueueReadBuffer(d_output, true, 0, size, h_outputGpu.data(), NULL, NULL);

    // write the image to a file
    if (writePPMImage("../data/fruit_copy.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

    // create a copy of a structure
    ppm_t imgCPU2;
    // copy the image
    imgCPU2.width = imgCPU.width;
    imgCPU2.height = imgCPU.height;
    imgCPU2.data = (rgb_pixel_t *)malloc(imgCPU.width * imgCPU.height * sizeof(rgb_pixel_t));
    memcpy(imgCPU2.data, imgCPU.data, imgCPU.width * imgCPU.height * sizeof(rgb_pixel_t));

    // remove the blue channel from the image
    removeRedChannel(&imgCPU2);

    // write the image to a file
    if (writePPMImage("../data/fruit_no_blue.ppm", imgCPU2.width, imgCPU2.height, imgCPU2.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

    // test getter and setter functions
    setPixelR(&imgCPU2, 0, 0, 1);
    setPixelG(&imgCPU2, 0, 0, 2);
    setPixelB(&imgCPU2, 0, 0, 3);
    std::cout << "R: " << (int)getPixelR(&imgCPU2, 0, 0) << std::endl;
    std::cout << "G: " << (int)getPixelG(&imgCPU2, 0, 0) << std::endl;
    std::cout << "B: " << (int)getPixelB(&imgCPU2, 0, 0) << std::endl;

    // perform CSC
    performCSC(&imgCPU);

	// print the y, cb, cr values of the first pixel from imgCPU
	std::cout << "Values of the first pixel from imgCPU:" << std::endl;
	std::cout << "Y: " << (int)getPixelR(&imgCPU, 0, 0) << std::endl;
	std::cout << "Cb: " << (int)getPixelG(&imgCPU, 0, 0) << std::endl;
	std::cout << "Cr: " << (int)getPixelB(&imgCPU, 0, 0) << std::endl;
	// last pixel 
	std::cout << "Values of the last pixel from imgCPU:" << std::endl;
	std::cout << "Y: " << (int)getPixelR(&imgCPU, imgCPU.width - 1, imgCPU.height - 1) << std::endl;
	std::cout << "Cb: " << (int)getPixelG(&imgCPU, imgCPU.width - 1, imgCPU.height - 1) << std::endl;
	std::cout << "Cr: " << (int)getPixelB(&imgCPU, imgCPU.width - 1, imgCPU.height - 1) << std::endl;

	// print the y, cb, cr values of the first pixel from d_outputGpu
	std::cout << "Values of the first pixel from d_outputGpu:" << std::endl;
	std::cout << "Y: " << (int)h_outputGpu[0] << std::endl;
	std::cout << "Cb: " << (int)h_outputGpu[imgCPU.width * imgCPU.height] << std::endl;
	std::cout << "Cr: " << (int)h_outputGpu[2 * imgCPU.width * imgCPU.height] << std::endl;
	// last pixel
	std::cout << "Values of the last pixel from d_outputGpu:" << std::endl;
	std::cout << "Y: " << (int)h_outputGpu[imgCPU.width * imgCPU.height - 1] << std::endl;
	std::cout << "Cb: " << (int)h_outputGpu[2 * imgCPU.width * imgCPU.height - 1] << std::endl;
	std::cout << "Cr: " << (int)h_outputGpu[3 * imgCPU.width * imgCPU.height - 1] << std::endl;


    // write the image to a file
    if (writePPMImage("../data/fruit_csc.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

    // perform CDS
    performCDS(&imgCPU);

    // write the image to a file
    if (writePPMImage("../data/fruit_cds.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }
	
	// get 8x8 divisible image size
	size_t newWidth, newHeight;
	getNearest8x8ImageSize(imgCPU.width, imgCPU.height, &newWidth, &newHeight);

	ppm_t imgCPU3;
	// copy the image
	imgCPU3.width = newWidth;
	imgCPU3.height = newHeight;
	imgCPU3.data = (rgb_pixel_t *)malloc(newWidth * newHeight * sizeof(rgb_pixel_t));
	copyToLargerImage(&imgCPU, &imgCPU3);

	previewImage(&imgCPU3, 248, 0, 8, 8);

	// write the image to a file
	if (writePPMImage("../data/fruit_copy_larger.ppm", imgCPU3.width, imgCPU3.height, imgCPU3.data) == -1) {
		std::cout << "Error writing the image" << std::endl;
		return 1;
	}

	// add padding to the image
	addReversedPadding(&imgCPU3, imgCPU.width, imgCPU.height);

	previewImage(&imgCPU3, 248, 0, 8, 8);

	// write the image to a file
	if (writePPMImage("../data/fruit_copy_larger_padded.ppm", imgCPU3.width, imgCPU3.height, imgCPU3.data) == -1) {
		std::cout << "Error writing the image" << std::endl;
		return 1;
	}

	ppm_d_t imgCPU_d;
	// copy the image
	imgCPU_d.width = imgCPU3.width;
	imgCPU_d.height = imgCPU3.height;
	imgCPU_d.data = (rgb_pixel_d_t *)malloc(imgCPU3.width * imgCPU3.height * sizeof(rgb_pixel_d_t));
	copyUIntToDoubleImage(&imgCPU3, &imgCPU_d);

	substractfromAll(&imgCPU_d, 128.0);

	previewImageD(&imgCPU_d, 248, 0, 8, 8);
	
	performDCT(&imgCPU_d);

	previewImageD(&imgCPU_d, 248, 0, 8, 8);

	performQuantization(&imgCPU_d, quant_mat_lum, quant_mat_chrom);

	previewImageD(&imgCPU_d, 248, 0, 8, 8);

	// Check whether results are correct
	std::size_t errorCount = 0;
	// for (size_t i = 0; i < countX; i = i + 1) { //loop in the x-direction
	// 	for (size_t j = 0; j < countY; j = j + 1) { //loop in the y-direction
	// 		size_t index = i + j * countX;
	// 		// Allow small differences between CPU and GPU results (due to different rounding behavior)
	// 		if (!(std::abs ((int64_t) h_outputCpu[index] - (int64_t) h_outputGpu[index]) <= maxError)) {
	// 			if (errorCount < 15)
	// 				std::cout << "Result for " << i << "," << j << " is incorrect: GPU value is " << h_outputGpu[index] << ", CPU value is " << h_outputCpu[index] << std::endl;
	// 			else if (errorCount == 15)
	// 				std::cout << "..." << std::endl;
	// 			errorCount++;
	// 		}
	// 	}
	// }
	if (errorCount != 0) {
		std::cout << "Found " << errorCount << " incorrect results" << std::endl;
		return 1;
	}

	std::cout << "Success" << std::endl;

	return 0;
}
