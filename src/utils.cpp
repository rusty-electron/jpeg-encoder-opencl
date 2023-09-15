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