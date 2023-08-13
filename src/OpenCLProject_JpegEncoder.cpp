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

//////////////////////////////////////////////////////////////////////////////
// CPU implementation
//////////////////////////////////////////////////////////////////////////////
struct rgb_pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

typedef struct rgb_pixel rgb_pixel_t;

struct PPMimage {
	size_t w, h;
	rgb_pixel_t *pixel;
};

typedef struct PPMimage ppm_t;

int readPPMImage(const char * const file_path, size_t *width, size_t *height, rgb_pixel_t **buffer) {
	char line[128];
	char *token;
	int return_val = 0;
	int max_value;

	// open the file in binary mode
	FILE *fp = fopen(file_path, "rb");
	// check if the file was opened successfully
	if (!fp) {
		std::cout << "input file NF";
		return_val = -1;
		goto cleanup;
	}

	// read the first line of the file
	// if it is not possible to read from the file, return -2
	if (!fgets(line, sizeof(line), fp)) {
		return_val = -2;
		goto cleanup;
	}

	// check if the file is a PPM file
	// the file has to start with the magic number P6
	if (strcmp(line, "P6\n")) {
		return_val = -3;
		goto cleanup;
	}

	while (fgets(line, sizeof(line), fp)) {
		// check if the line is a comment
		// comments start with a '#'
		if (line[0] == '#') {
			continue;
		} else {
			token = strtok(line, " ");
			*width = atoi(token);
			token = strtok(NULL, " "); // NULL to continue parsing the same string
			*height = atoi(token);
			// read the maximum value of a pixel and store it in max_value 
			// as an integer
			fgets(line, sizeof(line), fp);
			max_value = atoi(line);
			// check if the maximum value is 255
			if (max_value != 255) {
				return_val = -4;
				goto cleanup;
			}
			break;
		 }
	}
	// allocate memory for the image
	*buffer = (rgb_pixel_t *)malloc(*width * *height * sizeof(rgb_pixel_t));

	// check if the memory was allocated successfully
	if (*buffer == NULL) {
		return_val = -5;
		goto cleanup;
	}

	// read the image data from the file
	(void)fread(*buffer, sizeof(rgb_pixel_t), *width * *height, fp);

	cleanup:
	fclose(fp);
	std::cout << return_val;
	return return_val;
	//return 1;
}

void JpegEncoderHost(/* fill here*/) {
	// stub for CPU implementation

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

	// Create a kernel object
	cl::Kernel JpegEncoderKernel(program, "JpegEncoderKernel");
	
	// start here with your own code
	ppm_t buffer;
	
	if (readPPMImage("../data/fruit.ppm", &buffer.w, &buffer.h, &buffer.pixel)) {
		std::cout << "Error reading the image" << std::endl;
		return 1;
	}

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
