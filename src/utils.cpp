#include <iomanip>
#include <cmath>
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
	for (size_t y = 0; y < img->height; y += 2) {
		for (size_t x = 0; x < img->width; x += 2) {
			rgb_pixel_t *pixel1 = getPixelPtr(img, x, y);
			rgb_pixel_t *pixel2 = getPixelPtr(img, x + 1, y);
			rgb_pixel_t *pixel3 = getPixelPtr(img, x, y + 1);
			rgb_pixel_t *pixel4 = getPixelPtr(img, x + 1, y + 1);

			uint8_t avgCb = (pixel1->g + pixel2->g + pixel3->g + pixel4->g) / 4;
			uint8_t avgCr = (pixel1->b + pixel2->b + pixel3->b + pixel4->b) / 4;

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

void performDCT(ppm_t *img) {
	
}

rgb_pixel_t getPixel(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x];
}

rgb_pixel_t* getPixelPtr(ppm_t *img, size_t x, size_t y) {
	return &img->data[y * img->width + x];
}

uint8_t getPixelR(ppm_t *img, size_t x, size_t y) {
	return img->data[y * img->width + x].r;
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

void addSymmetricPadding(ppm_t *img, size_t newWidth, size_t newHeight) {
	// add padding to the right
	for (size_t y = 0; y < img->height; ++y) {
		rgb_pixel_t *pixel = getPixelPtr(img, img->width - 1, y);
		for (size_t x = img->width; x < newWidth; ++x) {
			setPixelR(img, x, y, pixel->r);
			setPixelG(img, x, y, pixel->g);
			setPixelB(img, x, y, pixel->b);
		}
	}

	// add padding to the bottom
	for (size_t x = 0; x < newWidth; ++x) {
		rgb_pixel_t *pixel = getPixelPtr(img, x, img->height - 1);
		for (size_t y = img->height; y < newHeight; ++y) {
			setPixelR(img, x, y, pixel->r);
			setPixelG(img, x, y, pixel->g);
			setPixelB(img, x, y, pixel->b);
		}
	}
}

// preview the image pixels

void previewImage(ppm_t *img, size_t startX = 0, size_t startY = 0, size_t lengthX = 8, size_t lengthY = 8) {
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