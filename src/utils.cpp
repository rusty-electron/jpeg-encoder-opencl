#include <iomanip>
#include <cmath>
#include <vector>

#include <OpenCL/cl-patched.hpp>
#include "huffman.hpp"
#include "utils.hpp"


// Read the PPM image from file
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

// Write the PPM image to file
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

// Function to remove the red channel from the image - TEST FUNCTION
void removeRedChannel(ppm_t *img) {
	for (size_t i = 0; i < img->width * img->height; ++i) {
		img->data[i].r = 0;
	}
}

// Function to perform Color Space Conversion
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

// Function to perform Color Downsampling
void performCDS(ppm_t *img) {
	/* CDS = Color Downsampling
	 * Cb = (Cb1 + Cb2 + Cb3 + Cb4) / 4
	 * Cr = (Cr1 + Cr2 + Cr3 + Cr4) / 4
	 * Input: YCbCr struct
	 * Output: Downsampling Cb and Cr channels (used 2x2 average)
	*/
	for (size_t y = 0; y < img->height - 1; y += 2) {
		for (size_t x = 0; x < img->width - 1; x += 2) {
			rgb_pixel_t *pixel1 = getPixelPtr(img, x, y);
			rgb_pixel_t *pixel2 = getPixelPtr(img, x + 1, y);
			rgb_pixel_t *pixel3 = getPixelPtr(img, x, y + 1);
			rgb_pixel_t *pixel4 = getPixelPtr(img, x + 1, y + 1);

			uint8_t avgCb = (uint8_t)((pixel1->g + pixel2->g + pixel3->g + pixel4->g) / 4.0);
			uint8_t avgCr = (uint8_t)((pixel1->b + pixel2->b + pixel3->b + pixel4->b) / 4.0);

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

// Function to extract pixels from the image
rgb_pixel_t getPixel(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x];
}

// Function to extract pixel pointer from the image
rgb_pixel_t* getPixelPtr(ppm_t *img, size_t x, size_t y) {
	return &img->data[y * img->width + x];
}

// Extract pixel R value
uint8_t getPixelR(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].r; // img->height
}

// Extract pixel G value
uint8_t getPixelG(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].g;
}

// Extract pixel B value
uint8_t getPixelB(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].b;
}

// Set pixel R value
void setPixelR(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].r = val;
}

// Set pixel G value
void setPixelG(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].g = val;
}

// Set pixel B value
void setPixelB(ppm_t *img, size_t x, size_t y, uint8_t val) {
	img->data[y * img->width + x].b = val;
}

// Function to get the nearest 8x8 image size for padding
void getNearest8x8ImageSize(size_t width, size_t height, size_t *newWidth, size_t *newHeight) {
	*newWidth = std::ceil(width / 8.0) * 8;
	*newHeight = std::ceil(height / 8.0) * 8;
}

// Function for Level Shifting
void substractfromAll(ppm_d_t *img, double val) {
	for (size_t i = 0; i < img->width * img->height; ++i) {
		img->data[i].r -= val;
		img->data[i].g -= val;
		img->data[i].b -= val;
	}
}

// Function to copy the image to a larger image
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

// Function to add padding to the image
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

// Function to convert the uint to double image
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

// Function to convert the double to uint image
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

// Function to perform DCT on the image (loop over all MCU's)
void performDCT(ppm_d_t *img) {
	// perform DCT on each 8x8 block
	for (size_t y = 0; y < img->height; y += 8) {
		for (size_t x = 0; x < img->width; x += 8) {
			// perform DCT on the block
			performDCTBlock(img, x, y);
		}
	}
}

// Function to perform DCT on the image (loop over all MCU's) - Alternate function
void performDCT2(ppm_d_t *img) {
	for (size_t y = 0; y < img->height; ++y) {
		for (size_t x = 0; x < img->width; ++x) {
			size_t idx_within_block_x = x % 8;
			size_t idx_within_block_y = y % 8;

			float alphaU = (idx_within_block_x == 0) ? 1.0 / std::sqrt(2) : 1.0;
			float alphaV = (idx_within_block_y == 0) ? 1.0 / std::sqrt(2) : 1.0;

			float sumR = 0.0;
			float sumG = 0.0;
			float sumB = 0.0;

			int startX = (x / 8) * 8;
			int startY = (y / 8) * 8;

			for (int m = 0; m < 8; m++) {
				for (int n = 0; n < 8; n++) {
					float cosX = std::cos((2 * m + 1) * idx_within_block_x * M_PI / 16.0);
					float cosY = std::cos((2 * n + 1) * idx_within_block_y * M_PI / 16.0);

					rgb_pixel_d_t pixel = img->data[(startY + n) * img->width + (startX + m)];
					sumR += (float)pixel.r * cosX * cosY;
					sumG += (float)pixel.g * cosX * cosY;
					sumB += (float)pixel.b * cosX * cosY;
				}
			}

			sumR *= (alphaU * alphaV * 0.25);
			sumG *= (alphaU * alphaV * 0.25);
			sumB *= (alphaU * alphaV * 0.25);

			rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
			pixel->r = sumR;
			pixel->g = sumG;
			pixel->b = sumB;
		}
	}
}

// Function to perform DCT on a single 8x8 block (MCU)
void performDCTBlock(ppm_d_t *img, size_t startX, size_t startY) {
	// perform DCT on each channel
	for (size_t u = 0; u < 8; ++u) {
		for (size_t v = 0; v < 8; ++v) {
			double alphaU = (u == 0) ? 1.0 / std::sqrt(2) : 1.0;
			double alphaV = (v == 0) ? 1.0 / std::sqrt(2) : 1.0;

			double sumR = 0.0;
			double sumG = 0.0;
			double sumB = 0.0;

			for (size_t y = startY; y < startY + 8; ++y) {
				size_t eff_y = y - startY;
				for (size_t x = startX; x < startX + 8; ++x) {
					rgb_pixel_d_t *pixel = &img->data[y * img->width + x];
					size_t eff_x = x - startX;
					sumR += pixel->r * std::cos((2 * eff_x + 1) * u * M_PI / 16.0) * std::cos((2 * eff_y + 1) * v * M_PI / 16.0);
					sumG += pixel->g * std::cos((2 * eff_x + 1) * u * M_PI / 16.0) * std::cos((2 * eff_y + 1) * v * M_PI / 16.0);
					sumB += pixel->b * std::cos((2 * eff_x + 1) * u * M_PI / 16.0) * std::cos((2 * eff_y + 1) * v * M_PI / 16.0);
				}
			}

			sumR *= (alphaU * alphaV / 4.0);
			sumG *= (alphaU * alphaV / 4.0);
			sumB *= (alphaU * alphaV / 4.0);

			int x_val = startX + u;
			int y_val = startY + v;
			rgb_pixel_d_t *pixel = &img->data[y_val * img->width + x_val];
			pixel->r = sumR;
			pixel->g = sumG;
			pixel->b = sumB;
		}
	}
}


// Function to preview the image from the ppm_t struct
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

// Function to preview the image from the ppm_d_t struct
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

// Function to print a message
void printMsg(std::string msg) {
	if (msg != "") {
		std::cout << "## " << msg << " ##" << std::endl;
	}
}

//  Function to preview assuming the input is uint8_t
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
// Function to preview assuming the input is int
void previewImageLinearI(std::vector<int>& v, const unsigned int width, const unsigned int height, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8, std::string msg) {
	// print message if provided
	printMsg(msg);

	std::cout << "Previewing pixels from (" << startX << ", " << startY << ") to (" << startX + lengthX - 1 << ", " << startY + lengthY - 1 << "):" << std::endl;
	size_t idx;
	int val1, val2, val3;
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

// Function to preview assuming the input is double
void previewImageLinearD(std::vector<float>& v, const unsigned int width, const unsigned int height, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8, std::string msg) {
	// print message if provided
	printMsg(msg);

	std::cout << "Previewing pixels from (" << startX << ", " << startY << ") to (" << startX + lengthX - 1 << ", " << startY + lengthY - 1 << "):" << std::endl;
	size_t idx;
	float val1, val2, val3;
	for (size_t y = startY; y < startY + lengthY; ++y) {
		for (size_t x = startX; x < startX + lengthX; ++x) {
			idx = y * width + x;
			val1 = v[idx];
			val2 = v[idx + width * height];
			val3 = v[idx + width * height * 2];
			// provide three spaces for each pixel
			printf("(%6.2f, %6.2f, %6.2f)   ", val1, val2, val3);
		}
		std::cout << std::endl;
	}
}

// Function to perform quantization on the image
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

// Function to perform quantization on the image - Alternate function
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

// Function to concetanate every MCU of the three channels into a single 2D array
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


void everyMCUisnow1DArray(std::vector<int>& input_arr, int output_arr[], unsigned int width, unsigned int height) {
	unsigned int rowsPerChannel = height * width / 64;
	unsigned int numOfMCUsX = width / 8;
	for (size_t y = 0; y < height; y += 8) {
		for (size_t x = 0; x < width; x += 8) {
			for (size_t v = 0; v < 8; ++v) {
				for (size_t u = 0; u < 8; ++u) {
					output_arr[v * 8 + u + (y / 8 * numOfMCUsX + x / 8) * 64] = input_arr[(y + v) * width + (x + u)];
					output_arr[v * 8 + u + (y / 8 * numOfMCUsX + x / 8 + rowsPerChannel) * 64] = input_arr[(y + v) * width + (x + u) + width * height];
					output_arr[v * 8 + u + (y / 8 * numOfMCUsX + x / 8 + rowsPerChannel * 2) * 64] = input_arr[(y + v) * width + (x + u) + width * height * 2];
				}
			}
		}
	}
}

// Function to perform diagonal zigzag traversal on an 2D linearized array and store the result in a 1D array 
// method : Reconstructing the linearized array into a 2D array and then performing the diagonal zigzag traversal
void diagonalZigZagBlock(int linear_arr[], int zigzag_arr[]) {
    int index = 0; // Index for zigzag_arr
	int MCU[8][8];
	for (int i = 0; i < 64; i++) {
		MCU[i / 8][i % 8] = linear_arr[i];
	}

    for (int diag = 0; diag < 16 - 1; ++diag) {
        const auto i_min = std::max(0, diag - 8 + 1);
        const auto i_max = i_min + std::min(diag, 8 - 1);

        for (auto i = i_min; i <= i_max; ++i) {
            const auto row = diag % 2 ? i : (diag - i);
            const auto col = diag % 2 ? (diag - i) : i;
			zigzag_arr[index++] = MCU[row][col];
		}
			
    }
}
// Function to perform diagonal zigzag traversal directly on the linearized array
void diagonalZigZagBlockLinear(int linear_arr[], int zigzag_arr[]) {
	unsigned int index = 0;
	for (int diag = 0; diag < 16 - 1; ++diag) {
		const auto i_min = std::max(0, diag - 8 + 1);
		const auto i_max = i_min + std::min(diag, 2*(8 - 1) - diag);

		for (auto i = i_min; i <= i_max; ++i) {
			const auto row = diag % 2 ? i : (diag - i);
			const auto col = diag % 2 ? (diag - i) : i;
			zigzag_arr[index++] = linear_arr[row * 8 + col];
		}
	}
}

// Function to perform zigzag traversal across the MCU blocks
void performZigZag(int linear_arr[][64], int zigzag_arr[][64], int numRows) {
	for (size_t i = 0; i < numRows; ++i) {
		diagonalZigZagBlockLinear(linear_arr[i], zigzag_arr[i]);
	}
}

// Seperate the zigzag array into three parts for each channel
void seperateChannels(int zigzag_arr[][64], int zigzag_arr_y[][64], int zigzag_arr_cb[][64], int zigzag_arr_cr[][64], int numRowsPerChannel) {
	for (size_t i = 0; i < numRowsPerChannel; ++i) {
		for (size_t j = 0; j < 64; ++j) {
			zigzag_arr_y[i][j] = zigzag_arr[i][j];
			zigzag_arr_cb[i][j] = zigzag_arr[i+numRowsPerChannel][j];
			zigzag_arr_cr[i][j] = zigzag_arr[i+(numRowsPerChannel*2)][j];
		}
	}
}

// Function to perform RLE on the zigzag array on AC coefficients
void RLEBlockAC(int zigzag_array[], std::vector<int>& rle_vector) {
  	int count = 0;
	int lastNonZeroIndex = 0;

	// Get the index of the last non-zero element
	for (int i = 63; i >= 0; i--) {
		if (zigzag_array[i] != 0) {
			lastNonZeroIndex=i; // Return the index of the last non-zero element
			break;
		}
	}

    // RLE for the AC components
    for (int i = 1; i <= lastNonZeroIndex; i++) {
        // Count the number of zeros before each element
        if (zigzag_array[i] == 0) {
			if (count == 15) {
				// Store the number of zeros
				rle_vector.push_back(count);
				// Store the value of the non-zero element
				rle_vector.push_back(0);
				count = 0;
			} else {
            	count++;
			}
		}
		else 
		{		
			// Store the number of zeros
			rle_vector.push_back(count);
			// Store the value of the non-zero element
			rle_vector.push_back(zigzag_array[i]);
			count = 0;
		}
    }
	rle_vector.push_back(0);
    rle_vector.push_back(0);
}

// Function to perform RLE on all the MCU blocks
void performRLE(int zigzag_array[][64], std::vector<std::vector<int>>& rle_vector, int rowsperchannel)
{
	for(size_t i=0; i <rowsperchannel; i++)
	{
		std::vector<int> newRow;
		RLEBlockAC(zigzag_array[i], newRow);
		rle_vector.push_back(newRow);
	}
}

// Function to get the category from the given value
const int16_t getValueCategory(const int16_t value) {
    if (value == 0x0000)
        return 0;
    return std::log2(std::abs(value)) + 1;
}

// Function to convert the value to a bit stream
const std::string valueToBitString(const int16_t value) {
    if (value == 0x0000)
        //return "0"; 
        return "";
    
    int16_t val = value;
    int bits = ( std::log2( std::abs( value ) ) ) + 1;
    //std::cout << "Bits: " << bits << ", ";
    std::string bitStr( bits, '0' );
    
    if (val < 0) {
        int16_t delta = 0xFFFF >> (16 - bits);
        val += delta;
        val = std::abs( val );
    }
    
    int i = bitStr.size() - 1;
    while (val > 0) {
        bitStr[i--] = '0' + val % 2;
        val /= 2;
    }
    
    return bitStr;
}

// Function to implement the Huffman encoding
std::string HuffmanEncoder(int zigzag_array[][64], 
					std::vector<std::vector<int>>& rle_vector,
					int numRowsPerChannel) {

	std::string m_scandata = "";
	int dc_components[numRowsPerChannel][3];

	// Take the difference between the first element of two consecutive MCU blocks 
	// and store the difference in the first element of the second MCU block
	int lastVal[3] = {0, 0, 0};
	// For DC coefficient
	for (size_t i = 0; i < numRowsPerChannel; ++i) {
		for (size_t chan = 0; chan < 3; ++chan) {
			// Encode the DC coefficients
			dc_components[i][chan] = zigzag_array[i + (numRowsPerChannel * chan)][0] - lastVal[chan];
			lastVal[chan] = zigzag_array[i + (numRowsPerChannel * chan)][0];

			auto category = getValueCategory(dc_components[i][chan]);
			auto bitString = valueToBitString(dc_components[i][chan]);

			if (chan == 0) {
				m_scandata += DC_LUMA_HUFF_CODES[category] + bitString;
			} else {
				m_scandata += DC_CHROMA_HUFF_CODES[category] + bitString;
			}

			// Encode the AC coefficients
			for (size_t j = 0; j < rle_vector[i + (numRowsPerChannel * chan)].size(); j+=2) {
				auto zero_run = rle_vector[i + (numRowsPerChannel * chan)][j];
				auto value = rle_vector[i + (numRowsPerChannel * chan)][j+1];
				auto category = getValueCategory(value);
				auto bitString = valueToBitString( value );

				if (chan == 0) {
					m_scandata += AC_LUMA_HUFF_CODES[zero_run][category] + bitString;
				} else {
					m_scandata += AC_CHROMA_HUFF_CODES[zero_run][category] + bitString;
				}
			}
		}
	}		
	return m_scandata;			
}

// Function to perform the copy Image to vector
void copyImageToVector(ppm_t *img, std::vector <cl_uint>& v) {
	for (size_t idx = 0; idx < img->width * img->height; ++idx) {
		v[idx] = img->data[idx].r;
		v[idx + img->width * img->height] = img->data[idx].g;
		v[idx + img->width * img->height * 2] = img->data[idx].b;
	}
}

// Function to implement Reverse Padding for GPU
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

// Function to restrucure the vector in RGBRGBRGB... to RRR...GGG...BBB...
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
// Write ppm image to file (GPU)
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