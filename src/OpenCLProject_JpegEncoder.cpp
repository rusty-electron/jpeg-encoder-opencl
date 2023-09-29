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
#include <iomanip>

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

// void dct(uint8_t* image, size_t width, size_t height, uint8_t *header){
// 	const double sqrt2 = std::sqrt(2.0);
//     const double PI = 3.14159265359;
// 	double *dctImg;

//     dctImg = new double [width * height];

//     for (size_t u = 0; u < height; ++u) {
//         for (size_t v = 0; v < width; ++v) {
//             double sum = 0.0;

//             for (size_t x = 0; x < height; ++x) {
//                 for (size_t y = 0; y < width; ++y) {
//                     double cu = (x == 0) ? 1.0 / sqrt2 : 1.0;
//                     double cv = (y == 0) ? 1.0 / sqrt2 : 1.0;

//                     size_t index = (x * width + y) * 3; // Start of YCbCr pixel
//                     double yVal = static_cast<double>(image[index]); // Y channel
//                     double cbVal = static_cast<double>(image[index + 1]); // Cb channel
//                     double crVal = static_cast<double>(image[index + 2]); // Cr channel

//                     sum += cu * cv * yVal * std::cos((2.0 * u + 1.0) * x * PI / (2.0 * height)) *
//                            std::cos((2.0 * v + 1.0) * y * PI / (2.0 * width));
//                 }
//             }

//             dctImg[u * width + v] = sum;
//         }
// 	}
// 	FILE *write;
// 	write = fopen("../data/dct.ppm","wb");
// 	writePPM(write, header, dctImg, (int)height*width*3);
// }

int JpegEncoderHost(ppm_t imgCPU, CPUTelemetry *cpu_telemetry = NULL) {
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

    // perform CSC
	Core::TimeSpan startTime = Core::getCurrentTime();
    performCSC(&imgCPU);
	Core::TimeSpan endTime = Core::getCurrentTime();
	Core::TimeSpan CSCTimeCPU = endTime - startTime;
	std::cout << "CSC Time CPU: " << CSCTimeCPU.toString() << std::endl;

	// write the image to a file
    if (writePPMImage("../data/fruit_csc.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

    // perform CDS
	startTime = Core::getCurrentTime();
    performCDS(&imgCPU);
	endTime = Core::getCurrentTime();
	Core::TimeSpan CDSTimeCPU = endTime - startTime;
	std::cout << "CDS Time CPU: " << CDSTimeCPU.toString() << std::endl;

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

	startTime = Core::getCurrentTime();
	copyToLargerImage(&imgCPU, &imgCPU3);
	endTime = Core::getCurrentTime();
	Core::TimeSpan copyTimeCPU = endTime - startTime;

	// write the image to a file
	if (writePPMImage("../data/fruit_copy_larger.ppm", imgCPU3.width, imgCPU3.height, imgCPU3.data) == -1) {
		std::cout << "Error writing the image" << std::endl;
		return 1;
	}

	// TODO: add padding to the image only if the image dims are not divisible by 8
	// add padding to the image
	addReversedPadding(&imgCPU3, imgCPU.width, imgCPU.height);

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

	startTime = Core::getCurrentTime();
	copyUIntToDoubleImage(&imgCPU3, &imgCPU_d);
	endTime = Core::getCurrentTime();

	Core::TimeSpan copyTimeCPU2 = endTime - startTime;
	Core::TimeSpan TotalCopyTimeCPU = copyTimeCPU + copyTimeCPU2;
	std::cout << "Total Copy Time CPU: " << TotalCopyTimeCPU.toString() << std::endl;

	startTime = Core::getCurrentTime();
	substractfromAll(&imgCPU_d, 128.0);

	performDCT(&imgCPU_d);
	endTime = Core::getCurrentTime();

	Core::TimeSpan DCTTimeCPU = endTime - startTime;
	std::cout << "DCT Time CPU: " << DCTTimeCPU.toString() << std::endl;

	startTime = Core::getCurrentTime();
	performQuantization(&imgCPU_d, quant_mat_lum, quant_mat_chrom);
	endTime = Core::getCurrentTime();

	Core::TimeSpan QuantTimeCPU = endTime - startTime;
	std::cout << "Quantization Time CPU: " << QuantTimeCPU.toString() << std::endl;

	previewImageD(&imgCPU_d, 0, 0, 8, 8);

	// initialize an linear array with the size of the image
	float *image = new float[imgCPU_d.width * imgCPU_d.height * 3];

	// number of rows of 2D array = (total image pixels / 64) * 3
	// number of columns of 2D array = 64
	unsigned int rows = (imgCPU_d.width * imgCPU_d.height) / 64 * 3;
	unsigned int rowsperchannel = (imgCPU_d.width * imgCPU_d.height) / 64;
	int linear_arr[rows][64];
	int zigzag_arr[rows][64];
	
	startTime = Core::getCurrentTime();
	everyMCUisnow2DArray(&imgCPU_d, linear_arr);

	// print the first row of the 2D array
	// std::cout << "First row of the 2D array:" << std::endl;
	// for (int i = 0; i < 64; i++) {
	// 	std::cout << std::setw(3) << linear_arr[1024][i] << " ";
	// 	if ((i + 1) % 8 == 0) {
	// 		std::cout << std::endl;
	// 	}
	// }
	// std::cout << std::endl;

	// perform zigzag on the 2D array
	performZigZag(linear_arr, zigzag_arr, rows);
	endTime = Core::getCurrentTime();

	Core::TimeSpan ZigZagTimeCPU = endTime - startTime;
	std::cout << "ZigZag Time CPU: " << ZigZagTimeCPU.toString() << std::endl;

	// Seperate channels
	int zigzag_y[rowsperchannel][64];
	int zigzag_cb[rowsperchannel][64];
	int zigzag_cr[rowsperchannel][64];

	seperateChannels(zigzag_arr, zigzag_y, zigzag_cb, zigzag_cr, rowsperchannel);
	
	// print the first row of the zigzag_y array
	std::cout << "First row of the zigzag_y array:" << std::endl;
	for (int i = 0; i < 64; i++) {
		std::cout << zigzag_y[0][i] << " ";
	}
	std::cout<<std::endl;

	// 2D vector to store the rle values
	std::vector<std::vector<int>> y_rle;
	std::vector<std::vector<int>> cb_rle;
	std::vector<std::vector<int>> cr_rle;

	
	startTime = Core::getCurrentTime();
	// For y channel
	performRLE(zigzag_y, y_rle, rowsperchannel);
	// For cb channel
	performRLE(zigzag_cb, cb_rle, rowsperchannel);
	// For cr channel
	performRLE(zigzag_cr, cr_rle, rowsperchannel);
	endTime = Core::getCurrentTime();

	Core::TimeSpan RLETimeCPU = endTime - startTime;
	std::cout << "RLE Time CPU: " << RLETimeCPU.toString() << std::endl;



	// Print first row of y RLE
	std::cout << "First row of the y RLE:" << std::endl;
	for (int i = 0; i < y_rle[0].size(); i+=2) {
		std::cout << "(" << y_rle[0][i] << ", " << y_rle[0][i+1] << ") ";
	}

	// copy telemetry data to the structure
	if (cpu_telemetry != NULL) {
		cpu_telemetry->CSCTime = static_cast<double>(CSCTimeCPU.getMicroseconds());
		cpu_telemetry->CDSTime = static_cast<double>(CDSTimeCPU.getMicroseconds());
		cpu_telemetry->DCTTime = static_cast<double>(DCTTimeCPU.getMicroseconds());
		cpu_telemetry->TotalCopyTime = static_cast<double>(TotalCopyTimeCPU.getMicroseconds());
		cpu_telemetry->QuantTime = static_cast<double>(QuantTimeCPU.getMicroseconds());
		cpu_telemetry->zigZagTime = static_cast<double>(ZigZagTimeCPU.getMicroseconds());
		cpu_telemetry->RLETime = static_cast<double>(RLETimeCPU.getMicroseconds());
	}

	// print total time
	std::cout << "Total Time CPU: " << (CSCTimeCPU + CDSTimeCPU + TotalCopyTimeCPU + DCTTimeCPU + QuantTimeCPU + ZigZagTimeCPU + RLETimeCPU).toString() << std::endl;

	return 0;
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

	copyImageToVector(&imgCPU, h_input);

	// create an instance of cpu_telemetry
	CPUTelemetry cpu_telemetry;
	// perform the JPEG encoding on the CPU
	JpegEncoderHost(imgCPU, &cpu_telemetry);

	// Copy input data to device
	queue.enqueueWriteBuffer(d_input, true, 0, size, h_input.data(), NULL, NULL);

	previewImageLinear(h_input, imgCPU.width, imgCPU.height, 0, 0, 8, 8, "Before colorConversionKernel():");

	cl::Event colorConversionEvent;
	// create a kernel object for color conversion
	cl::Kernel colorConversionKernel(program, "colorConversionKernel");

	// Set kernel parameters
	colorConversionKernel.setArg<cl::Buffer>(0, d_input);
	colorConversionKernel.setArg<cl::Buffer>(1, d_output);
	// convert size_t to unsigned int
	colorConversionKernel.setArg<cl_uint>(2, (cl_uint)imgCPU.width);
	colorConversionKernel.setArg<cl_uint>(3, (cl_uint)imgCPU.height);

	// Launch kernel on the compute device
	queue.enqueueNDRangeKernel(colorConversionKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, &colorConversionEvent);
	// Copy output data back to host
	queue.enqueueReadBuffer(d_output, true, 0, size, h_outputGpu.data(), NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	// Print performance data
	Core::TimeSpan colorConversionTimeGPU = OpenCL::getElapsedTime(colorConversionEvent);
	std::cout << "Color conversion time (GPU): " << colorConversionTimeGPU.toString() << std::endl;

	previewImageLinear(h_outputGpu, imgCPU.width, imgCPU.height, 248, 0, 8, 8, "After colorConversionKernel():");

	/* Copy to larger image */
	// get 8x8 divisible image size
	size_t newWidth, newHeight;
	getNearest8x8ImageSize(imgCPU.width, imgCPU.height, &newWidth, &newHeight);

	// create a new and larger vector to store the new image
	std::vector<cl_uint> h_largeimg (count);
	// create a larger output vector to store the new output image
	std::vector<cl_uint> h_largeoutput (count);

	// at this point, the original image vector has been converted to YCbCr
	// and extra padding has been added to the image to make the image dims divisible by 8
	copyOntoLargerVectorWithPadding(h_outputGpu, h_largeimg, imgCPU.width, imgCPU.height, newWidth, newHeight);

	previewImageLinear(h_largeimg, newWidth, newHeight, 248, 0, 8, 8, "After copyOntoLargerVectorWithPadding():");

	queue.enqueueWriteBuffer(d_input, true, 0, size, h_largeimg.data(), NULL, NULL);

	memset(h_largeoutput.data(), 255, size);

	queue.enqueueWriteBuffer(d_output, true, 0, size, h_largeoutput.data(), NULL, NULL);

	cl::Event chromaSubsamplingEvent;
	// create a kernel object for chroma subsampling
	cl::Kernel chromaSubsamplingKernel(program, "chromaSubsamplingKernel");
	chromaSubsamplingKernel.setArg<cl::Buffer>(0, d_input);
	chromaSubsamplingKernel.setArg<cl::Buffer>(1, d_output);
	chromaSubsamplingKernel.setArg<cl_uint>(2, (cl_uint)newWidth);
	chromaSubsamplingKernel.setArg<cl_uint>(3, (cl_uint)newHeight);

	// Launch kernel on the compute device
	queue.enqueueNDRangeKernel(chromaSubsamplingKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, &chromaSubsamplingEvent);

	// Copy output data back to host
	queue.enqueueReadBuffer(d_output, true, 0, size, h_largeoutput.data(), NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	// Print performance data
	Core::TimeSpan chromaSubsamplingTimeGPU = OpenCL::getElapsedTime(chromaSubsamplingEvent);
	std::cout << "Chroma subsampling time (GPU): " << chromaSubsamplingTimeGPU.toString() << std::endl;

	// testing 
	std::vector<cl_uint> h_img_test(newWidth * newHeight * 3);
	switchVectorChannelOrdering(h_largeoutput, h_img_test, newWidth, newHeight);
	writeVectorToFile("../data/fruit_gpu_subsampling_output.ppm", newWidth, newHeight, h_img_test);

	// previewImageLinear(h_outputGpu, imgCPU.width, imgCPU.height, 248, 0, 8, 8, "After chromaSubsamplingKernel():");
	// print the first 4 chroma pixels
	// std::cout << "First 4 chroma subsampled pixels:" << std::endl;
	// std::cout << "Cr1 " << (int)h_outputGpu[imgCPU.width * imgCPU.height] << std::endl;
	// std::cout << "Cr2 " << (int)h_outputGpu[imgCPU.width * imgCPU.height + 1] << std::endl;
	// std::cout << "Cr3 " << (int)h_outputGpu[imgCPU.width * imgCPU.height + imgCPU.width] << std::endl;
	// std::cout << "Cr4 " << (int)h_outputGpu[imgCPU.width * imgCPU.height + imgCPU.width + 1] << std::endl;

	// create a vector of type *float* to store newInput data
	std::vector<float> h_newinput (h_outputGpu.begin(), h_outputGpu.end());
	// create a vector to store newOutput data
	std::vector<int> h_newoutput (size);
	// create a vector a fill it with quantization matrix for luminance
	std::vector<cl_uint> h_quant_mat_lum (64);
	// copy the quantization matrix for luminance
	for (size_t i = 0; i < 64; ++i) {
		h_quant_mat_lum[i] = quant_mat_lum[i / 8][i % 8];
	}
	// create a vector a fill it with quantization matrix for chrominance
	std::vector<cl_uint> h_quant_mat_chrom (64);
	// copy the quantization matrix for chrominance
	for (size_t i = 0; i < 64; ++i) {
		h_quant_mat_chrom[i] = quant_mat_chrom[i / 8][i % 8];
	}

	memset(h_newoutput.data(), 255, size);

	// allocate buffer for newInput data
	cl::Buffer d_finput = cl::Buffer(context, CL_MEM_READ_WRITE, size * sizeof (cl_float));
	// allocate buffer for newOutput data
	cl::Buffer d_foutput = cl::Buffer(context, CL_MEM_READ_WRITE, size * sizeof (int));
	// allocate buffer for quantization matrix for luminance
	cl::Buffer d_matA = cl::Buffer(context, CL_MEM_READ_ONLY, 64 * sizeof (cl_uint));
	// allocate buffer for quantization matrix for chrominance
	cl::Buffer d_matB = cl::Buffer(context, CL_MEM_READ_ONLY, 64 * sizeof (cl_uint));

	// write newInput data to device
	queue.enqueueWriteBuffer(d_finput, true, 0, size * sizeof (cl_float), h_newinput.data(), NULL, NULL);
	// write quantization matrix for luminance to device
	queue.enqueueWriteBuffer(d_matA, true, 0, 64 * sizeof (cl_uint), h_quant_mat_lum.data(), NULL, NULL);
	// write quantization matrix for chrominance to device
	queue.enqueueWriteBuffer(d_matB, true, 0, 64 * sizeof (cl_uint), h_quant_mat_chrom.data(), NULL, NULL);
	// write newOutput data to device
	queue.enqueueWriteBuffer(d_foutput, true, 0, size * sizeof (int), h_newoutput.data(), NULL, NULL);

	// create a kernel object for quantization
	cl::Kernel quantizationKernel(program, "quantizationKernel");
	quantizationKernel.setArg<cl::Buffer>(0, d_finput);
	quantizationKernel.setArg<cl::Buffer>(1, d_foutput);
	quantizationKernel.setArg<cl::Buffer>(2, d_matA);
	quantizationKernel.setArg<cl::Buffer>(3, d_matB);
	quantizationKernel.setArg<cl_uint>(4, (cl_uint)imgCPU.width);
	quantizationKernel.setArg<cl_uint>(5, (cl_uint)imgCPU.height);

	// Launch quantization kernel on the compute device
	queue.enqueueNDRangeKernel(quantizationKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, NULL);
	// Copy output data back to host
	queue.enqueueReadBuffer(d_foutput, true, 0, size * sizeof (int), h_newoutput.data(), NULL, NULL);

	// TODO: use single index for performing zigzag on the GPU
	int zigzagInput[newWidth * newHeight * 3];
	int zigzagOutput[newWidth * newHeight * 3];

	everyMCUisnow1DArray(h_newoutput, zigzagInput, newWidth, newHeight);

	previewImageLinearI(h_newoutput, newWidth, newHeight, 0, 0, 8, 8, "Before ZigZag():");

	// print first row of zigzagInput
	std::cout << "First row of zigzagInput:" << std::endl;
	for (int i = 0; i < 64; i++) {
		std::cout << zigzagInput[i] << " ";
	}
	std::cout << std::endl;

	unsigned int dims = newWidth * newHeight * 3;
	// allocate buffer for zigzagInput data
	cl::Buffer d_zigzagInput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims);
	// allocate buffer for zigzagOutput data
	cl::Buffer d_zigzagOutput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims);

	// write zigzagInput data to device
	queue.enqueueWriteBuffer(d_zigzagInput, true, 0, dims * sizeof (int), zigzagInput, NULL, NULL);
	
	cl::Event zigzagEvent;
	// create a kernel object for zigzag
	cl::Kernel zigzagKernel(program, "zigzagKernel");
	zigzagKernel.setArg<cl::Buffer>(0, d_zigzagInput);
	zigzagKernel.setArg<cl::Buffer>(1, d_zigzagOutput);

	// Launch zigzag kernel on the compute device
	queue.enqueueNDRangeKernel(zigzagKernel, cl::NullRange, cl::NDRange(dims / 64, 64), cl::NDRange(wgSizeX, wgSizeY), NULL, &zigzagEvent);
	// Copy output data back to host
	queue.enqueueReadBuffer(d_zigzagOutput, true, 0, dims * sizeof (int), zigzagOutput, NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	Core::TimeSpan zigzagTimeGPU = OpenCL::getElapsedTime(zigzagEvent);
	std::cout << "ZigZag time (GPU): " << zigzagTimeGPU.toString() << std::endl;

	// print first row of zigzagOutput
	std::cout << "\nFirst row of zigzagOutput:" << std::endl;
	for (int i = 0; i < 64; i++) {
		std::cout << zigzagOutput[i] << " ";
	}
	std::cout << std::endl;

	// Run Length Encoding
	int rleOutput[dims * 2];

	// allocate buffer for rle step
	cl::Buffer d_rleInput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims);
	// allocate buffer for rleOutput data
	cl::Buffer d_rleOutput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims * 2);

	// write zigzagOutput data to device
	queue.enqueueWriteBuffer(d_rleInput, true, 0, dims * sizeof (int), zigzagOutput, NULL, NULL);

	cl::Event rleEvent;
	// create a kernel object for rle
	cl::Kernel rleKernel(program, "rleKernel");
	rleKernel.setArg<cl::Buffer>(0, d_rleInput);
	rleKernel.setArg<cl::Buffer>(1, d_rleOutput);

	// run length encoding in single index
	queue.enqueueNDRangeKernel(rleKernel, 0, dims / 64, 64, NULL, &rleEvent);

	// Copy output data back to host
	queue.enqueueReadBuffer(d_rleOutput, true, 0, dims * sizeof (int) * 2, rleOutput, NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	Core::TimeSpan rleTimeGPU = OpenCL::getElapsedTime(rleEvent);
	std::cout << "RLE time (GPU): " << rleTimeGPU.toString() << std::endl;

	// print first row of rleOutput
	std::cout << "\nFirst row of rleOutput:" << std::endl;
	for (int i = 0; i < 64 * 2; i+=2) {
		if (rleOutput[i] == 0 && rleOutput[i+1] == 0) {
			break;
		}
		std::cout << "(" << rleOutput[i] << ", " << rleOutput[i+1] << ") ";
	}

	// Calculate speedups
	std::cout << "\n## Speedups: ##" << std::endl;
	std::cout << "Color conversion: " << (cpu_telemetry.CSCTime / static_cast<double>(colorConversionTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "Chroma subsampling: " << (cpu_telemetry.CDSTime / static_cast<double>(chromaSubsamplingTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "ZigZag: " << (cpu_telemetry.zigZagTime / static_cast<double>(zigzagTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "RLE: " << (cpu_telemetry.RLETime / static_cast<double>(rleTimeGPU.getMicroseconds())) << std::endl;

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
