#include <iomanip>
#include <cmath>
#include <vector>

#include <OpenCL/cl-patched.hpp>

#include "utils.hpp"

int readPPMImage(const char * file_path, size_t *width, size_t *height, rgb_pixel_t **imgptr) {
	char line[128];
	FILE *fp = fopen(file_path, "rb");

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

		unsigned int img_dims = *width * *height;

		// assign memory to the image
		*imgptr = (rgb_pixel_t *)malloc(img_dims * sizeof(rgb_pixel_t));

		if (*imgptr == NULL) {
			std::cout << "Error allocating memory" << std::endl;
			fclose(fp);
			return -1;
		}

		fread(*imgptr, sizeof(rgb_pixel_t), img_dims, fp);
		fclose(fp);
	} else {
		std::cout << "Error opening the file" << std::endl;
		return -1;
	}
	return 0;
}

int writePPMImage(const char * file_path, size_t width, size_t height, rgb_pixel_t *imgptr) {
    FILE *fp = fopen(file_path, "wb");

    if (fp) {
        fprintf(fp, "P6\n");
        fprintf(fp, "%d %d\n", width, height);
        fprintf(fp, "255\n");
        fwrite(imgptr, sizeof(rgb_pixel_t), width * height, fp);
        fclose(fp);
    } else {
        std::cout << "Error opening the file" << std::endl;
        return -1;
    }
    return 0;
}

void removeRedChannel(ppm_t *img) {
	for (size_t i = 0; i < img->width * img->height; ++i) {
		img->data[i].r = 0;
	}
}

void performCSC(ppm_t *img) {
	/* CSC = Color Space Conversion
	 * Y = 0.299 * R + 0.587 * G + 0.114 * B
	 * Cb = -0.168736 * R - 0.331264 * G + 0.5 * B + 128
	 * Cr = 0.5 * R - 0.418688 * G - 0.081312 * B + 128 
	 * Input: RGB struct
	 * Output: Convert RGB to YCbCr color space
	*/ 
	for (size_t i = 0; i < img->width * img->height; ++i) {
		rgb_pixel_t *pixel = &img->data[i];
		uint8_t r = pixel->r;
		uint8_t g = pixel->g;
		uint8_t b = pixel->b;

		pixel->r = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
		pixel->g = (uint8_t)(-0.168736 * r - 0.331264 * g + 0.5 * b + 128);
		pixel->b = (uint8_t)(0.5 * r - 0.418688 * g - 0.081312 * b + 128);
	}
}

void performCDS(ppm_t *img) {
	/* CDS = Color Downsampling
	 * Cb = (Cb1 + Cb2 + Cb3 + Cb4) / 4
	 * Cr = (Cr1 + Cr2 + Cr3 + Cr4) / 4
	 * Input: YCbCr struct
	 * Output: Downsampling Cb and Cr channels (used 2x2 average)
	*/
	for (size_t y = 0; y < img->height; y += 2) {
		for (size_t x = 0; x < img->width; x += 2) {
			rgb_pixel_t *pixel1 = getPixelPtr(img, x, y);
			rgb_pixel_t *pixel2 = getPixelPtr(img, x + 1, y);
			rgb_pixel_t *pixel3 = getPixelPtr(img, x, y + 1);
			rgb_pixel_t *pixel4 = getPixelPtr(img, x + 1, y + 1);

			uint8_t avgCb = (pixel1->g + pixel2->g + pixel3->g + pixel4->g) / 4.0;
			uint8_t avgCr = (pixel1->b + pixel2->b + pixel3->b + pixel4->b) / 4.0;

			pixel1->g = avgCb;
			pixel2->g = avgCb;
			pixel3->g = avgCb;
			pixel4->g = avgCb;

			pixel1->b = avgCr;
			pixel2->b = avgCr;
			pixel3->b = avgCr;
			pixel4->b = avgCr;
		}
	}
}

rgb_pixel_t getPixel(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x];
}

rgb_pixel_t* getPixelPtr(ppm_t *img, size_t x, size_t y) {
	return &img->data[y * img->width + x];
}

uint8_t getPixelR(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].r; // img->height
}

uint8_t getPixelG(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].g;
}

uint8_t getPixelB(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].b;
}

void setPixelR(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].r = val;
}

void setPixelG(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].g = val;
}

void setPixelB(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].b = val;
}

void getNearest8x8ImageSize(size_t width, size_t height, size_t *newWidth, size_t *newHeight) {
	*newWidth = std::ceil(width / 8.0) * 8;
	*newHeight = std::ceil(height / 8.0) * 8;
}

void substractfromAll(ppm_d_t *img, double val) {
	for (size_t i = 0; i < img->width * img->height; ++i) {
		img->data[i].r -= val;
		img->data[i].g -= val;
		img->data[i].b -= val;
	}
}

void copyToLargerImage(ppm_t *img, ppm_t *newImg) {
	for (size_t y = 0; y < img->height; ++y) {
		for (size_t x = 0; x < img->width; ++x) {
			rgb_pixel_t *pixel = getPixelPtr(img, x, y);
			setPixelR(newImg, x, y, pixel->r);
			setPixelG(newImg, x, y, pixel->g);
			setPixelB(newImg, x, y, pixel->b);
		}
	}
}

void addReversedPadding(ppm_t *img, size_t oldWidth, size_t oldHeight) {
	// add padding to the right
	for (size_t y = 0; y < oldHeight; ++y) {
		for (size_t x = oldWidth; x < img->width; ++x) {
			int diff = x - oldWidth + 1;
			rgb_pixel_t *pixel = getPixelPtr(img, oldWidth - diff, y);
			setPixelR(img, x, y, pixel->r);
			setPixelG(img, x, y, pixel->g);
			setPixelB(img, x, y, pixel->b);
		}
	}

	// add padding to the bottom
	for (size_t y = oldHeight; y < img->height; ++y) {
		for (size_t x = 0; x < img->width; ++x) {
			int diff = y - oldHeight + 1;
			rgb_pixel_t *pixel = getPixelPtr(img, x, oldHeight - diff);
			setPixelR(img, x, y, pixel->r);
			setPixelG(img, x, y, pixel->g);
			setPixelB(img, x, y, pixel->b);
		}
	}
}

void copyUIntToDoubleImage(ppm_t *img, ppm_d_t *newImg) {
	for (size_t y = 0; y < img->height; ++y) {
		for (size_t x = 0; x < img->width; ++x) {
			rgb_pixel_t *pixel = getPixelPtr(img, x, y);
			rgb_pixel_d_t *newPixel = &newImg->data[y * img->width + x];
			newPixel->r = (double)pixel->r;
			newPixel->g = (double)pixel->g;
			newPixel->b = (double)pixel->b;
		}
	}
}

void copyDoubleToUIntImage(ppm_d_t *img, ppm_t *newImg) {
	for (size_t y = 0; y < img->height; ++y) {
		for (size_t x = 0; x < img->width; ++x) {
			rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
			rgb_pixel_t *newPixel = getPixelPtr(newImg, x, y);
			newPixel->r = (uint8_t)pixel->r;
			newPixel->g = (uint8_t)pixel->g;
			newPixel->b = (uint8_t)pixel->b;
		}
	}
}

void performDCT(ppm_d_t *img) {
	// perform DCT on each 8x8 block
	for (size_t y = 0; y < img->height; y += 8) {
		for (size_t x = 0; x < img->width; x += 8) {
			// perform DCT on the block
			performDCTBlock(img, x, y);
		}
	}
}

void performDCTBlock(ppm_d_t *img, size_t startX, size_t startY) {
	// perform DCT on each channel
	for (size_t u = 0; u < 8; ++u) {
		for (size_t v = 0; v < 8; ++v) {
			double alphaU = (u == 0) ? 1.0 : 1.0 / std::sqrt(2);
			double alphaV = (v == 0) ? 1.0 : 1.0 / std::sqrt(2);

			double sumR = 0.0;
			double sumG = 0.0;
			double sumB = 0.0;

			for (size_t y = startY; y < startY + 8; ++y) {
				for (size_t x = startX; x < startX + 8; ++x) {
					rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
					sumR += pixel->r * std::cos((2 * x + 1) * u * M_PI / 16.0) * std::cos((2 * y + 1) * v * M_PI / 16.0);
					sumG += pixel->g * std::cos((2 * x + 1) * u * M_PI / 16.0) * std::cos((2 * y + 1) * v * M_PI / 16.0);
					sumB += pixel->b * std::cos((2 * x + 1) * u * M_PI / 16.0) * std::cos((2 * y + 1) * v * M_PI / 16.0);
				}
			}

			sumR *= (alphaU * alphaV / 4.0);
			sumG *= (alphaU * alphaV / 4.0);
			sumB *= (alphaU * alphaV / 4.0);
			// sumR *= 1/4.0;
			// sumG *= 1/4.0;
			// sumB *= 1/4.0;

			int x_val = startX + u;
			int y_val = startY + v;
			rgb_pixel_d_t *pixel = &img->data[y_val * img->width + x_val];
			pixel->r = sumR;
			pixel->g = sumG;
			pixel->b = sumB;
		}
	}
}

// preview the image pixels

void previewImage(ppm_t *img, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8, std::string msg) {
	// print message if provided
	printMsg(msg);

	std::cout << "Previewing pixels from (" << startX << ", " << startY << ") to (" << startX + lengthX - 1 << ", " << startY + lengthY - 1 << "):" << std::endl;
	for (size_t y = startY; y < startY + lengthY; ++y) {
		for (size_t x = startX; x < startX + lengthX; ++x) {
			rgb_pixel_t *pixel = getPixelPtr(img, x, y);
			// provide three spaces for each pixel
			std::cout << "(" << std::setw(3) << (int)pixel->r << ", " << std::setw(3) << (int)pixel->g << ", " << std::setw(3) << (int)pixel->b << ")   ";
		}
		std::cout << std::endl;
	}
}

void previewImageD(ppm_d_t *img, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8, std::string msg) {
	// print message if provided
	printMsg(msg);

	std::cout << "Previewing pixels from (" << startX << ", " << startY << ") to (" << startX + lengthX - 1 << ", " << startY + lengthY - 1 << "):" << std::endl;
	for (size_t y = startY; y < startY + lengthY; ++y) {
		for (size_t x = startX; x < startX + lengthX; ++x) {
			rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
			// provide three spaces for each pixel
			// only 2 decimal places
			printf("(%6.2f,%6.2f,%6.2f) ", pixel->r, pixel->g, pixel->b);
		}
		std::cout << std::endl;
	}
}

void printMsg(std::string msg) {
	if (msg != "") {
		std::cout << "## " << msg << " ##" << std::endl;
	}
}

void previewImageLinear(std::vector <cl_uint>& v, const unsigned int width, const unsigned int height, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8, std::string msg) {
	// print message if provided
	printMsg(msg);

	std::cout << "Previewing pixels from (" << startX << ", " << startY << ") to (" << startX + lengthX - 1 << ", " << startY + lengthY - 1 << "):" << std::endl;
	size_t idx;
	uint8_t val1, val2, val3;
	for (size_t y = startY; y < startY + lengthY; ++y) {
		for (size_t x = startX; x < startX + lengthX; ++x) {
			idx = y * width + x;
			val1 = v[idx];
			val2 = v[idx + width * height];
			val3 = v[idx + width * height * 2];
			// provide three spaces for each pixel
			std::cout << "(" << std::setw(3) << (int)val1 << ", " << std::setw(3) << (int)val2 << ", " << std::setw(3) << (int)val3 << ")   ";
		}
		std::cout << std::endl;
	}
}

// TODO: if the simpler (2-loop) GPU implementation works, modify this function to use only two loops instead of four
void performQuantization(ppm_d_t *img, const unsigned int quant_mat_lum[8][8], const unsigned int quant_mat_chrom[8][8]) {
	for (size_t y = 0; y < img->height; y += 8) {
		for (size_t x = 0; x < img->width; x += 8) {
			for (size_t v = 0; v < 8; ++v) {
				for (size_t u = 0; u < 8; ++u) {
					rgb_pixel_d_t *pixel = &img->data[(y + v) * img->width + (x + u)];
					pixel->r = std::round(pixel->r / quant_mat_lum[v][u]);
					pixel->g = std::round(pixel->g / quant_mat_chrom[v][u]);
					pixel->b = std::round(pixel->b / quant_mat_chrom[v][u]);
				}
			}
		}
	}
}

// TODO: test it!
void performQuantizationSimple(ppm_d_t *img, const unsigned int quant_mat_lum[8][8], const unsigned int quant_mat_chrom[8][8]) {
	for (size_t y = 0; y < img->height; y += 1) {
		for (size_t x = 0; x < img->width; x += 1) {
			rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
			pixel->r = std::round(pixel->r / quant_mat_lum[y % 8][x % 8]);
			pixel->g = std::round(pixel->g / quant_mat_chrom[y % 8][x % 8]);
			pixel->b = std::round(pixel->b / quant_mat_chrom[y % 8][x % 8]);
		}
	}
}

void everyMCUisnow2DArray(ppm_d_t *img, int linear_arr[][64]) {
	unsigned int rowsPerChannel = img->height * img->width / 64;
	unsigned int numOfMCUsX = img->width / 8;
	for (size_t y = 0; y < img->height; y += 8) {
		for (size_t x = 0; x < img->width; x += 8) {
			for (size_t v = 0; v < 8; ++v) {
				for (size_t u = 0; u < 8; ++u) {
					rgb_pixel_d_t *pixel = &img->data[(y + v) * img->width + (x + u)];
					
					linear_arr[y / 8 * numOfMCUsX + x / 8][v * 8 + u] = pixel->r;
					linear_arr[y / 8 * numOfMCUsX + x / 8 + rowsPerChannel][v * 8 + u] = pixel->g;
					linear_arr[y / 8 * numOfMCUsX + x / 8 + rowsPerChannel * 2][v * 8 + u] = pixel->b;
				}
			}
		}
	}
}

void performZigZag(int linear_arr[][64], int zigzag_arr[][64], int numRows) {
	for (size_t i = 0; i < numRows; ++i) {
		// performZigZagBlock(linear_arr[i], zigzag_arr[i]);
	}
}

// Function to perform diagonal zigzag traversal on an image matrix and store the result in a 1D array
void diagonalZigZag(ppm_d_t* img, float* zigzagOrder) {
    const int n = img->height;
    const int m = img->width;
    
    int index = 0; // Index for zigzagOrder
	rgb_pixel_d_t *pixels = img->data;

    for (int diag = 0; diag < n + m - 1; ++diag) {
        const auto i_min = std::max(0, diag - m + 1);
        const auto i_max = i_min + std::min(diag, n - 1);

        for (auto i = i_min; i <= i_max; ++i) {
            const auto row = diag % 2 ? i : (diag - i);
            const auto col = diag % 2 ? (diag - i) : i;
			if (index < img->width * img->height * 3) {
				// (row, col) is the current element
				zigzagOrder[index] = pixels[col * img->width + row].r;
				zigzagOrder[index + 1] = pixels[col * img->width + row].g;
				zigzagOrder[index + 2] = pixels[col * img->width + row].b;

				index += 3;
			}
        }
    }
	//Print the last 30 elements of the zigzag order
	int numElements = img->width * img->height * 3;
	for (int i = 0; i < 40; i++) {
		printf("%f ", zigzagOrder[i]);
	}
}

// Seperate the zigzag order into 3 seperate arrays for each channel
void seperateChannels(ppm_d_t* img,float* zigzagOrder, float* y, float* cb, float* cr){

	int yIndex = 0;
    int cbIndex = 0;
    int crIndex = 0;

    for (int i =  0; i < img->width * img->height * 3; i += 3) {
        y[yIndex] = zigzagOrder[i];
        cb[cbIndex] = zigzagOrder[i + 1];
        cr[crIndex] = zigzagOrder[i + 2];

        yIndex++;
       	cbIndex++;
        crIndex++;
    }
}
// RLE implementation
void RLE(float* channel, float* RLEArray, int channelSize) {
    int count = 1;
    int index = 0;

    for (int i = 0; i < channelSize - 1; i++) {
        if (channel[i] == channel[i + 1]) {
            count++;
        } else {
            RLEArray[index] = count;
            RLEArray[index + 1] = static_cast<int>(channel[i]);
            index += 2;
            count = 1;
        }
    }

    // Handle the last run
    RLEArray[index] = count;
    RLEArray[index + 1] = static_cast<int>(channel[channelSize - 1]);
    // index += 2; // This will be the RLEArraySize
}

void copyImageToVector(ppm_t *img, std::vector <cl_uint>& v) {
	for (size_t idx = 0; idx < img->width * img->height; ++idx) {
		v[idx] = img->data[idx].r;
		v[idx + img->width * img->height] = img->data[idx].g;
		v[idx + img->width * img->height * 2] = img->data[idx].b;
	}
}

void copyOntoLargerVectorWithPadding(std::vector <cl_uint>& vInput, std::vector <cl_uint>& vOutput, const unsigned int oldWidth, const unsigned int oldHeight, const unsigned int newWidth, const unsigned int newHeight) {
	// copy the original image
	for (size_t y = 0; y < oldHeight; ++y) {
		for (size_t x = 0; x < oldWidth; ++x) {
			vOutput[y * newWidth + x] = vInput[y * oldWidth + x];
			vOutput[y * newWidth + x + newWidth * newHeight] = vInput[y * oldWidth + x + oldWidth * oldHeight];
			vOutput[y * newWidth + x + newWidth * newHeight * 2] = vInput[y * oldWidth + x + oldWidth * oldHeight * 2];
		}
	}

	// add padding to the right
	for (size_t y = 0; y < oldHeight; ++y) {
		for (size_t x = oldWidth; x < newWidth; ++x) {
			int diff = x - oldWidth + 1;
			vOutput[y * newWidth + x] = vInput[y * oldWidth + oldWidth - diff];
			vOutput[y * newWidth + x + newWidth * newHeight] = vInput[y * oldWidth + oldWidth - diff + oldWidth * oldHeight];
			vOutput[y * newWidth + x + newWidth * newHeight * 2] = vInput[y * oldWidth + oldWidth - diff + oldWidth * oldHeight * 2];
		}
	}

	// add padding to the bottom
	for (size_t y = oldHeight; y < newHeight; ++y) {
		for (size_t x = 0; x < newWidth; ++x) {
			int diff = y - oldHeight + 1;
			vOutput[y * newWidth + x] = vInput[(oldHeight - diff) * oldWidth + x];
			vOutput[y * newWidth + x + newWidth * newHeight] = vInput[(oldHeight - diff) * oldWidth + x + oldWidth * oldHeight];
			vOutput[y * newWidth + x + newWidth * newHeight * 2] = vInput[(oldHeight - diff) * oldWidth + x + oldWidth * oldHeight * 2];
		}
	}
}

// TODO: is this function still required?
void switchVectorChannelOrdering(std::vector <cl_uint>& vInput, std::vector <cl_uint>& vOutput, const unsigned int width, const unsigned int height) {
	for (size_t y = 0; y < height * width; ++y) {
		// place first channel in every third position starting with 0
		vOutput[y * 3] = vInput[y];
		// place second channel in every third position starting with 1
		vOutput[y * 3 + 1] = vInput[y + width * height];
		// place third channel in every third position starting with 2
		vOutput[y * 3 + 2] = vInput[y + width * height * 2];
	}
}

void writeVectorToFile(const char * file_path, const unsigned int width, const unsigned int height, std::vector <cl_uint>& imgVector) {
	// create file object 
	FILE *fp = fopen(file_path, "wb");

	if (fp) {
		fprintf(fp, "P6\n");
		fprintf(fp, "%d %d\n", width, height);
		fprintf(fp, "255\n");
		// loop through the vector and write the values to the file
		for (size_t i = 0; i < 3 * width * height; ++i) {
			uint8_t val = imgVector[i];
			fwrite(&val, sizeof(uint8_t), 1, fp);
		}
		fclose(fp);
	} else {
		std::cout << "Error opening the file" << std::endl;
	}
}
