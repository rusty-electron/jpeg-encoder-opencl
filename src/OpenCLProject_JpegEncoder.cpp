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
int getDimension(uint8_t *header, int &pos){
    int dim=0;
    for ( ;header[pos]!='\n' && header[pos]!=' ';pos++)
        dim = dim * 10 + (header[pos] - '0');
    return dim;
}

//function for test only
void removeBlue(uint8_t *image, uint8_t *withoutblueimage, int size){
    size_t i;
    for (i=0;i<size;i=i+3){
        withoutblueimage[i]=image[i];
        withoutblueimage[i+1]=image[i+1];
        withoutblueimage[i+2]=0;//blue component is set to 0
		//std::cout << int(image[i]) << " " << i;
    }
}

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


void cds(uint8_t* image, size_t width, size_t height, uint8_t *header){ 
	
	 for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            int index1 = (y * width + x) * 3;
            int index2 = (y * width + x + 1) * 3;
            int index3 = ((y + 1) * width + x) * 3;
            int index4 = ((y + 1) * width + x + 1) * 3;

            uint8_t avgCb = (image[index1 + 1] + image[index2 + 1] + image[index3 + 1] + image[index4 + 1]) / 4;
            uint8_t avgCr = (image[index1 + 2] + image[index2 + 2] + image[index3 + 2] + image[index4 + 2]) / 4;

            image[index1 + 1] = avgCb;
            image[index2 + 1] = avgCb;
            image[index3 + 1] = avgCb;
            image[index4 + 1] = avgCb;

            image[index1 + 2] = avgCr;
            image[index2 + 2] = avgCr;
            image[index3 + 2] = avgCr;
            image[index4 + 2] = avgCr;
        }
    }

	FILE *write;
	write = fopen("../data/cds.ppm","wb");
	writePPM(write, header, image, (int)height*width*3);


}

void csc(uint8_t* image, size_t size, uint8_t *header, size_t width, size_t height){
	uint8_t *cscImage;
	size_t i;
	for (i=0;i<size;i=i+3){
		
		//image[i] = 0.299*image[i];
		image[i] = static_cast<uint8_t>(0.299 * image[i] + 0.587 * image[i+1] + 0.114 * image[i+2]);
		image[i+1] = static_cast<uint8_t>(128 - 0.168736 * image[i] - 0.331264 * image[i+1] + 0.5 * image[i+2]);
		image[i+2] = static_cast<uint8_t>(128 + 0.5 * image[i] - 0.418688 * image[i+1] - 0.081312 * image[i+2]);
	}
	FILE *newF;
	newF = fopen("../data/csc.ppm","wb");
	writePPM(newF, header, image, (int)size);
	
	//
	
}

size_t easyPPMRead(uint8_t *image){
	FILE *read, *write;
	read = fopen("../data/fruit.ppm", "rb");
	uint8_t header[15];
	fread(header, 15, 1, read);
	if (header[0] != 'P' || header[1] != '6'){
		std::cout << "Wrong file format";
	}

	int width, height, clrs, pos = 3;
	width = getDimension(header, pos);
	pos++;
	height = getDimension(header, pos);
	std::cout << "Width:" << width << "\tHeight:" << height << '\n';
	image = new uint8_t [width * height * 3];
	
	fread(image, width*height*3, 1, read);
	uint8_t *withoutblueimage;
	withoutblueimage = new uint8_t [width * height * 3];
	removeBlue(image, withoutblueimage, width*height*3); //testing, to be removed later
	write = fopen("../data/west_1_without_blue.ppm","wb");
	writePPM(write, header, withoutblueimage, width*height*3);
	fclose(read);
	csc(image, width * height * 3, header, width, height);
	cds(image, width, height, header);
	dct(image, width, height, header);
	return static_cast<size_t>(width * height * 3);
	//return write;

}

void JpegEncoderHost(uint8_t *image) {
	// stub for CPU implementation
	size_t size;
	size = easyPPMRead(image);
	
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
	uint8_t *imgCPU;
	size_t width, height;
	
	if (readPPMImage("../data/fruit.ppm", &width, &height, &imgCPU) == -1) {
		std::cout << "Error reading the image" << std::endl;
		return 1;
	}

    // write the image to a file
    if (writePPMImage("../data/fruit_copy.ppm", width, height, imgCPU) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

	// uint8_t *imgPtr = &image;
	// int size;
	// size = easyPPMRead(imgPtr);
	// csc(imgPtr, size);
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
