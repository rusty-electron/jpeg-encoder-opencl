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

int JpegEncoderHost(ppm_t imgCPU, CPUTelemetry *cpu_telemetry = NULL) {
	
	// write the image to a file
    if (writePPMImage("../data/fruit_copy.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }
	// create a copy of the ppm_t structure
    ppm_t imgCPU2;
    
	// get the attributes of the image structure
    imgCPU2.width = imgCPU.width;
    imgCPU2.height = imgCPU.height;
    imgCPU2.data = (rgb_pixel_t *)malloc(imgCPU.width * imgCPU.height * sizeof(rgb_pixel_t));
	// copy the image
    memcpy(imgCPU2.data, imgCPU.data, imgCPU.width * imgCPU.height * sizeof(rgb_pixel_t));

    
	// remove the blue channel from the image - testing
    removeRedChannel(&imgCPU2);
	// write the image to a file - testing
    if (writePPMImage("../data/fruit_no_blue.ppm", imgCPU2.width, imgCPU2.height, imgCPU2.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }

	////////////////////////// Color Space Conversion //////////////////////////
    
	// perform color space conversion (CSC) and get the CPU time
	Core::TimeSpan startTime = Core::getCurrentTime();
    performCSC(&imgCPU);
	Core::TimeSpan endTime = Core::getCurrentTime();
	Core::TimeSpan CSCTimeCPU = endTime - startTime;
	std::cout << "CSC Time CPU: " << CSCTimeCPU.toString() << std::endl;
	
	// write the image after CSC to a file
    if (writePPMImage("../data/fruit_csc.ppm", imgCPU.width, imgCPU.height, imgCPU.data) == -1) {
        std::cout << "Error writing the image" << std::endl;
        return 1;
    }
	///////////////////////////////////////////////////////////////////////////
    
	//////////////////////// Chroma Downsampling ///////////////////////////////
	
	// perform chroma downsampling (CDS) and get the CPU time
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
	/////////////////////////////////////////////////////////////////////////////

	//////////////////////// Reverse Padding /////////////////////////////////////
	
	// get 8x8 divisible image size
	size_t newWidth, newHeight; 

	// Adjust the image size to be divisible by 8
	if (imgCPU.width % 8 == 0 && imgCPU.height % 8 == 0) {
		newWidth = imgCPU.width;
		newHeight = imgCPU.height;
	} else {
		getNearest8x8ImageSize(imgCPU.width, imgCPU.height, &newWidth, &newHeight);
	}

	ppm_t imgCPU3;
	
	// copy the image
	imgCPU3.width = newWidth;
	imgCPU3.height = newHeight;
	imgCPU3.data = (rgb_pixel_t *)malloc(newWidth * newHeight * sizeof(rgb_pixel_t));

	// copy the image to the new image with padding
	startTime = Core::getCurrentTime();
	copyToLargerImage(&imgCPU, &imgCPU3);
	endTime = Core::getCurrentTime();
	Core::TimeSpan copyTimeCPU = endTime - startTime;

	// write the image to a file
	if (writePPMImage("../data/fruit_copy_larger.ppm", imgCPU3.width, imgCPU3.height, imgCPU3.data) == -1) {
		std::cout << "Error writing the image" << std::endl;
		return 1;
	}

	// add reverse padding to the image
	addReversedPadding(&imgCPU3, imgCPU.width, imgCPU.height);

	// write the image to a file
	if (writePPMImage("../data/fruit_copy_larger_padded.ppm", imgCPU3.width, imgCPU3.height, imgCPU3.data) == -1) {
		std::cout << "Error writing the image" << std::endl;
		return 1;
	}
 
 	/////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////// Level Shifting /////////////////////////////////////////////////
	
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
	endTime = Core::getCurrentTime();

	std::cout << "First 8x8 block of the image after level shifting:" << std::endl;
	previewImageD(&imgCPU_d, 0, 0, 8, 8);

	Core::TimeSpan levelShiftingCPU = endTime - startTime;
	std::cout << "Level Shifting Time CPU: " << levelShiftingCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Discrete Cosine Transform ///////////////////////////////////

	startTime = Core::getCurrentTime();
	performDCT(&imgCPU_d);
	endTime = Core::getCurrentTime();

	Core::TimeSpan DCTTimeCPU = endTime - startTime;
	std::cout << "DCT Time CPU: " << DCTTimeCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Quantization ////////////////////////////////////////////////

	startTime = Core::getCurrentTime();
	performQuantization(&imgCPU_d, quant_mat_lum, quant_mat_chrom);
	endTime = Core::getCurrentTime();

	Core::TimeSpan QuantTimeCPU = endTime - startTime;
	std::cout << "Quantization Time CPU: " << QuantTimeCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////// ZigZag Scanning ////////////////////////////////////////////

	// initialize an linear array with the size of the image
	float *image = new float[imgCPU_d.width * imgCPU_d.height * 3];

	// number of rows of 2D array = (total image pixels / 64) * 3
	// number of columns of 2D array = 64
	unsigned int rows = (imgCPU_d.width * imgCPU_d.height) / 64 * 3;
	unsigned int rowsperchannel = (imgCPU_d.width * imgCPU_d.height) / 64;
	
	// make two 2D arrays to store the linear and zigzag arrays
	// where each row is a linearized MCU 
	int linear_arr[rows][64];
	int zigzag_arr[rows][64];

	startTime = Core::getCurrentTime();
	everyMCUisnow2DArray(&imgCPU_d, linear_arr);

	// perform zigzag on the 2D array
	performZigZag(linear_arr, zigzag_arr, rows);
	endTime = Core::getCurrentTime();

	Core::TimeSpan ZigZagTimeCPU = endTime - startTime;
	std::cout << "ZigZag Time CPU: " << ZigZagTimeCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// RLE Encoding ///////////////////////////////////////////////

	// 2D vector to store the rle values for all channels
	std::vector<std::vector<int>> rle;
	
	startTime = Core::getCurrentTime();

	// for all channels
	performRLE(zigzag_arr, rle, rows);
	endTime = Core::getCurrentTime();

	Core::TimeSpan RLETimeCPU = endTime - startTime;
	std::cout << "RLE Time CPU: " << RLETimeCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Huffman Encoding ////////////////////////////////////////////

	// huffman encoding
	startTime = Core::getCurrentTime();
	std::string scanData = HuffmanEncoder(zigzag_arr, rle, rowsperchannel);
	endTime = Core::getCurrentTime();

	Core::TimeSpan HuffmanTimeCPU = endTime - startTime;
	std::cout << "Huffman Time CPU: " << HuffmanTimeCPU.toString() << std::endl;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	// copy telemetry data to the structure
	if (cpu_telemetry != NULL) {
		cpu_telemetry->CSCTime = static_cast<double>(CSCTimeCPU.getMicroseconds());
		cpu_telemetry->CDSTime = static_cast<double>(CDSTimeCPU.getMicroseconds());
		cpu_telemetry->levelShiftTime = static_cast<double>(levelShiftingCPU.getMicroseconds());
		cpu_telemetry->DCTTime = static_cast<double>(DCTTimeCPU.getMicroseconds());
		cpu_telemetry->QuantTime = static_cast<double>(QuantTimeCPU.getMicroseconds());
		cpu_telemetry->zigZagTime = static_cast<double>(ZigZagTimeCPU.getMicroseconds());
		cpu_telemetry->RLETime = static_cast<double>(RLETimeCPU.getMicroseconds());
		cpu_telemetry->HuffmanTime = static_cast<double>(HuffmanTimeCPU.getMicroseconds());
		cpu_telemetry->TotalCopyTime = static_cast<double>(TotalCopyTimeCPU.getMicroseconds());
	}

	// print total time
	std::cout << "Total Time CPU: " << (CSCTimeCPU + CDSTimeCPU + TotalCopyTimeCPU + levelShiftingCPU + DCTTimeCPU + QuantTimeCPU + ZigZagTimeCPU + RLETimeCPU + HuffmanTimeCPU).toString() << std::endl;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	
	// Create a context
	//cl::Context context(CL_DEVICE_TYPE_GPU);
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.size() == 0) {
		std::cerr << "No platforms found" << std::endl;
		return 1;
	}
	// Select the platform
	int platformId = 0;
	for (size_t i = 0; i < platforms.size(); i++) {
		if (platforms[i].getInfo<CL_PLATFORM_NAME>() == "AMD Accelerated Parallel Processing") {
			platformId = i;
			break;
		}
	}
	// Create a context with the GPU device
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
	countX *= 3; // image channels
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

	// read the ppm image
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

	//////////////////////////////////// Color Space Conversion (GPU) ///////////////////////////////////////

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

	////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Chroma Subsampling (GPU) and Padding //////////////////////////////

	/* Copy to larger image */
	// get 8x8 divisible image size
	size_t newWidth, newHeight;
	if (imgCPU.width % 8 == 0 && imgCPU.height % 8 == 0) {
		newWidth = imgCPU.width;
		newHeight = imgCPU.height;
	} else {
		getNearest8x8ImageSize(imgCPU.width, imgCPU.height, &newWidth, &newHeight);
	}

	// create a new and larger vector to store the new image
	std::vector<cl_uint> h_largeimg (count);
	// create a larger output vector to store the new output image
	std::vector<cl_uint> h_largeoutput (count);

	// at this point, the original image vector has been converted to YCbCr
	// and extra padding has been added to the image to make the image dims divisible by 8
	copyOntoLargerVectorWithPadding(h_outputGpu, h_largeimg, imgCPU.width, imgCPU.height, newWidth, newHeight);

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

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Level Shifting and DCT (GPU) ///////////////////////////////////////

	// testing 
	std::vector<cl_uint> h_img_test(newWidth * newHeight * 3);
	switchVectorChannelOrdering(h_largeoutput, h_img_test, newWidth, newHeight);
	writeVectorToFile("../data/fruit_gpu_subsampling_output.ppm", newWidth, newHeight, h_img_test);

	// Level shift the chroma channels by 128
	std::vector<float> hDCTinput (h_largeoutput.begin(), h_largeoutput.end());
	std::vector<float> hDCTintermediate (count);

	// create buffers for DCT and level shifting
	cl::Buffer dDCTinput = cl::Buffer(context, CL_MEM_READ_WRITE, size * sizeof (float));
	cl::Buffer dDCTintermediate = cl::Buffer(context, CL_MEM_READ_WRITE, size * sizeof (float));

	// write DCT input data to device
	queue.enqueueWriteBuffer(dDCTinput, true, 0, count * sizeof (float), hDCTinput.data(), NULL, NULL);

	cl::Event LevelShiftEvent;
	// create a kernel object for level shifting
	cl::Kernel LevelShiftKernel(program, "LevelShiftKernel");
	LevelShiftKernel.setArg<cl::Buffer>(0, dDCTinput);
	LevelShiftKernel.setArg<cl::Buffer>(1, dDCTintermediate);
	LevelShiftKernel.setArg<cl_uint>(2, (cl_uint)newWidth);
	LevelShiftKernel.setArg<cl_uint>(3, (cl_uint)newHeight);

	// Launch kernel on the compute device
	queue.enqueueNDRangeKernel(LevelShiftKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, &LevelShiftEvent);

	// Copy output data back to host
	queue.enqueueReadBuffer(dDCTintermediate, true, 0, count * sizeof (float), hDCTintermediate.data(), NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	// Print performance data
	Core::TimeSpan LevelShiftTimeGPU = OpenCL::getElapsedTime(LevelShiftEvent);
	std::cout << "Level shift time (GPU): " << LevelShiftTimeGPU.toString() << std::endl;

	// perform DCT on the image
	std::vector<float> hDCToutput (count);

	// creat buffer for DCT output
	cl::Buffer dDCToutput = cl::Buffer(context, CL_MEM_READ_WRITE, count * sizeof (float));

	// copy DCT intermediate data to device (this will serve as input for DCT)
	queue.enqueueWriteBuffer(dDCTintermediate, true, 0, count * sizeof (float), hDCTintermediate.data(), NULL, NULL);

	cl::Event DCTEvent;
	// create a kernel object for DCT
	cl::Kernel DCTKernel(program, "DCTKernel");
	DCTKernel.setArg<cl::Buffer>(0, dDCTintermediate);
	DCTKernel.setArg<cl::Buffer>(1, dDCToutput);
	DCTKernel.setArg<cl_uint>(2, (cl_uint)newWidth);
	DCTKernel.setArg<cl_uint>(3, (cl_uint)newHeight);

	// Launch kernel on the compute device
	queue.enqueueNDRangeKernel(DCTKernel, cl::NullRange, cl::NDRange(countX, countY), cl::NDRange(wgSizeX, wgSizeY), NULL, &DCTEvent);

	// Copy output data back to host
	queue.enqueueReadBuffer(dDCToutput, true, 0, count * sizeof (float), hDCToutput.data(), NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	// Print performance data
	Core::TimeSpan DCTTimeGPU = OpenCL::getElapsedTime(DCTEvent);
	std::cout << "DCT time (GPU): " << DCTTimeGPU.toString() << std::endl;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// Quantization (GPU) ////////////////////////////////////////////////

	// create a vector of type *float* to store newInput data
	std::vector<float> h_newinput (hDCToutput.begin(), hDCToutput.end());
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

	cl::Event quantizationEvent;

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

	// Wait for all commands to complete
	queue.finish();

	// Print performance data
	Core::TimeSpan quantizationTimeGPU = OpenCL::getElapsedTime(quantizationEvent);
	std::cout << "Quantization time (GPU): " << quantizationTimeGPU.toString() << std::endl;

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// ZigZag Scanning (GPU) ///////////////////////////////////////////

	int zigzagInput[newWidth * newHeight * 3];
	int zigzagOutput[newWidth * newHeight * 3];

	everyMCUisnow1DArray(h_newoutput, zigzagInput, newWidth, newHeight);

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

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////// RLE Encoding (GPU) //////////////////////////////////////////////
	// Run Length Encoding
	int dims_for_rle = dims / 64 * 66;
	int rleOutput[dims_for_rle];

	// allocate buffer for rle step
	cl::Buffer d_rleInput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims);
	// allocate buffer for rleOutput data
	cl::Buffer d_rleOutput = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof (int) * dims_for_rle);

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
	queue.enqueueReadBuffer(d_rleOutput, true, 0, dims_for_rle * sizeof (int), rleOutput, NULL, NULL);

	// Wait for all commands to complete
	queue.finish();

	Core::TimeSpan rleTimeGPU = OpenCL::getElapsedTime(rleEvent);
	std::cout << "RLE time (GPU): " << rleTimeGPU.toString() << std::endl;

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	// Calculate speedups for all steps
	std::cout << "\n## Speedups: ##" << std::endl;
	std::cout << "Color conversion: " << (cpu_telemetry.CSCTime / static_cast<double>(colorConversionTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "Chroma subsampling: " << (cpu_telemetry.CDSTime / static_cast<double>(chromaSubsamplingTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "Level shifting: " << (cpu_telemetry.levelShiftTime / static_cast<double>(LevelShiftTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "DCT: " << (cpu_telemetry.DCTTime / static_cast<double>(DCTTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "Quantization: " << (cpu_telemetry.QuantTime / static_cast<double>(quantizationTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "ZigZag: " << (cpu_telemetry.zigZagTime / static_cast<double>(zigzagTimeGPU.getMicroseconds())) << std::endl;
	std::cout << "RLE: " << (cpu_telemetry.RLETime / static_cast<double>(rleTimeGPU.getMicroseconds())) << std::endl;


	return 0;
}
