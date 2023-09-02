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

int readPPMImage(const char* filepath, size_t* width, size_t* height, rgb_pixel_t** buffer) {
	char line[128];
	FILE *fp = fopen(filepath, "rb");

	if (fp) {
		if (!fgets(line, sizeof(line), fp)) {
			std::cout << "Error reading the file" << std::endl;
			fclose(fp);
			return -1;
		}

		if (strcmp(line, "P6\n")) {
			std::cout << "Invalid file format" << std::endl;
			fclose(fp);
			return -1;
		}

		while (fgets(line, sizeof(line), fp)) {
			if (line[0] == '#') {
				continue;
			} else {
				char *token = strtok(line, " ");
				*width = atoi(token);
				token = strtok(NULL, " ");
				*height = atoi(token);
				fgets(line, sizeof(line), fp);
				int max_value = atoi(line);
				if (max_value != 255) {
					std::cout << "Invalid maximum value" << std::endl;
					fclose(fp);
					return -1;
				}
				break;
			}
		}

		*buffer = (rgb_pixel_t *)malloc(*width * *height * sizeof(rgb_pixel_t));

		if (*buffer == NULL) {
			std::cout << "Error allocating memory" << std::endl;
			fclose(fp);
			return -1;
		}

		fread(*buffer, sizeof(rgb_pixel_t), *width * *height, fp);
		fclose(fp);
	} else {
		std::cout << "Error opening the file" << std::endl;
		return -1;
	}
	return 0;
}

uint8_t AccessPixel(ppm_t *image, size_t x, size_t y, size_t channel) {
	if (x >= image->w || y >= image->h) {
		return -1;
	} else {
		if (channel == 0) {
			return image->pixel[x + y * image->w].r;
		} else if (channel == 1) {
			return image->pixel[x + y * image->w].g;
		} else if (channel == 2) {
			return image->pixel[x + y * image->w].b;
		} else {
			return -1;
		}
	}
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
		return -1;
	}

	std::cout << "Image size: " << buffer.w << "x" << buffer.h << std::endl;
	// access first pixel
	std::cout << "First pixel: " << (int) AccessPixel(&buffer, 0, 0, 0) << "," << (int) AccessPixel(&buffer, 0, 0, 1) << "," << (int) AccessPixel(&buffer, 0, 0, 2) << std::endl;

	// free up memory
	free(buffer.pixel);

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
