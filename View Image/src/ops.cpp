#include "headers/ops.h"
#include "../resource.h"

#pragma region Memory

#define GetMemoryLocation(start, x, y, widthfactor) \
	((uint32_t*)(start) + ((y) * (widthfactor)) + (x))\
\

#pragma endregion

#pragma region File

bool isFile(const char* str, const char* suffix) {
	size_t len_str = strlen(str);
	size_t len_suffix = strlen(suffix);
	if (len_str < len_suffix) {
		return false;
	}
	for (size_t i = len_suffix; i > 0; i--) {
		if (tolower(str[len_str - i]) != tolower(suffix[len_suffix - i])) {
			return false;
		}
	}
	return true;
}

unsigned char* LoadImageFromResource(int resourceId, int& width, int& height, int& channels)
{
	// Get the instance handle of the current module
	HMODULE hModule = GetModuleHandle(nullptr);
	if (!hModule)
	{
		std::cerr << "Failed to get module handle" << std::endl;
		return nullptr;
	}

	// Find the resource by ID
	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDB_PNG1), "PNG");
	if (!hResource)
	{
		std::cerr << "Failed to find resource " << resourceId << std::endl;
		return nullptr;
	}

	// Load the resource data
	HGLOBAL hResourceData = LoadResource(hModule, hResource);
	if (!hResourceData)
	{
		std::cerr << "Failed to load resource data" << std::endl;
		return nullptr;
	}

	// Lock the resource data to get a pointer to the raw image data
	const void* resourceData = LockResource(hResourceData);
	if (!resourceData)
	{
		std::cerr << "Failed to lock resource data" << std::endl;
		FreeResource(hResourceData);
		return nullptr;
	}

	// Get the size of the resource data
	const size_t resourceSize = SizeofResource(hModule, hResource);

	// Load the image data from the resource data
	unsigned char* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(resourceData), resourceSize, &width, &height, &channels, 4);

	if (!imageData)
	{
		std::cerr << "Failed to load image from resource " << resourceId << ": " << stbi_failure_reason() << std::endl;
	}

	// Free the resource data
	FreeResource(hResourceData);

	return imageData;
}


#pragma endregion

#pragma region Math


void rotateImage90Degrees(GlobalParams* m) {
	// Update width and height after rotation
	size_t newWidth = m->imgheight;
	size_t newHeight = m->imgwidth;
	m->imgwidth = newWidth;
	m->imgheight = newHeight;

	// Allocate memory for rotated image
	uint32_t* rotatedImage = (uint32_t*)malloc(newWidth * newHeight * sizeof(uint32_t));

	// Transfer pixels to rotated image
	for (size_t y = 0; y < newHeight; y++) {
		for (size_t x = 0; x < newWidth; x++) {
			*GetMemoryLocation(rotatedImage, x,y, newWidth) = *GetMemoryLocation(m->imgdata,y, newWidth - 1 - x, m->imgheight);
		}
	}

	// Free memory of the original image
	free(m->imgdata);

	// Update pointer to point to rotated image
	m->imgdata = rotatedImage;

	RedrawImageOnBitmap(m);
}

int GetButtonInterval(GlobalParams* m) {
	return m->iconSize + 5;
}

int getXbuttonID(GlobalParams* m, POINT mPos) {
	LONG x = mPos.x;
	int interval = GetButtonInterval(m);
	return (mPos.y > m->toolheight || mPos.y < 2 || mPos.x < 2) ? -1 : x / interval;
}

float log_base_1_25(float x) {
	float log_1_25 = log(1.25);
	return log(x) / log_1_25;
}

float roundzoom(float z) {
	return pow(1.25f, round(log_base_1_25(z)));
}


void no_offset(GlobalParams* m) {
	m->iLocX = 0;
	m->iLocY = m->toolheight / 2;

	RedrawImageOnBitmap(m);
}

void autozoom(GlobalParams* m) {

	no_offset(m);

	float precentX = (float)m->width / (float)m->imgwidth;
	float precentY = (float)(m->height - m->toolheight) / (float)m->imgheight;

	float e = fmin(precentX, precentY);

	float fzoom = e;
	if (m->imgheight < 50) { fzoom = e / 2; }
	// round to the nearest power of 1.25 (for easy zooming back to 100)
	m->mscaler = fzoom;
	//if (mscaler > 1.0f && imgheight > 5) mscaler = 1.0f;

	RedrawImageOnBitmap(m);
}


void NewZoom(GlobalParams* m, float v, int mouse) {

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);
	p.x -= m->width / 2;
	p.y -= m->height / 2;

	if (mouse == 2) {
		p.x = 0;
		p.y = 0;
	}

	int distance_x = m->iLocX - p.x;
	int distance_y = m->iLocY - p.y;
	int new_width = m->width * v;
	int new_height = m->height * v;
	if (mouse) {

		m->iLocX = p.x + distance_x * v;
		m->iLocY = p.y + distance_y * v;
	}
	m->mscaler *= v;
	m->mscaler = roundzoom(m->mscaler);

	RedrawImageOnBitmap(m);
}

uint32_t InvertColorChannelsInverse(uint32_t d) {
	return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
}

uint32_t InvertColorChannels(uint32_t d) {
	return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
}

uint32_t lerp(uint32_t color1, uint32_t color2, float alpha)
{
	// Extract the individual color channels from the input values
	uint8_t a1 = (color1 >> 24) & 0xFF;
	uint8_t r1 = (color1 >> 16) & 0xFF;
	uint8_t g1 = (color1 >> 8) & 0xFF;
	uint8_t b1 = color1 & 0xFF;

	uint8_t a2 = (color2 >> 24) & 0xFF;
	uint8_t r2 = (color2 >> 16) & 0xFF;
	uint8_t g2 = (color2 >> 8) & 0xFF;
	uint8_t b2 = color2 & 0xFF;

	// Calculate the lerped color values for each channel
	uint8_t a = (1 - alpha) * a1 + alpha * a2;
	uint8_t r = (1 - alpha) * r1 + alpha * r2;
	uint8_t g = (1 - alpha) * g1 + alpha * g2;
	uint8_t b = (1 - alpha) * b1 + alpha * b2;

	// Combine the lerped color channels into a single 32-bit value
	return (a << 24) | (r << 16) | (g << 8) | b;
}



// Gaussian function
double gaussian(double x, double sigma) {
	return exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * 3.14159265) * sigma);
}


// Gaussian blur function
void gaussian_blur(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t offX, uint32_t offY) {
	// Compute kernel size
	int kernel_size = (int)ceil(sigma * 3) * 2 + 1;

	// Allocate kernel array
	double* kernel = (double*)malloc(kernel_size * sizeof(double));

	// Compute kernel values
	double sum = 0.0;
	int i;
	for (i = 0; i < kernel_size; i++) {
		double x = (double)i - (double)(kernel_size - 1) / 2.0;
		kernel[i] = gaussian(x, sigma);
		sum += kernel[i];
	}

	// Normalize kernel
	for (i = 0; i < kernel_size; i++) {
		kernel[i] /= sum;
	}

	// Allocate temporary row array
	uint32_t* row = (uint32_t*)malloc(width * sizeof(uint32_t));

	// Blur horizontally
	int x, y;
	for (y = 0; y < lH; y++) {
		for (x = 0; x < lW; x++) {
			int k;
			double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0;
			for (k = 0; k < kernel_size; k++) {
				int xk = x - (kernel_size - 1) / 2 + k;
				if (xk >= 0 && xk < width) {
					uint32_t pixel = pixels[(y + offY) * width + (xk + offX)];
					sum_r += (double)((pixel & 0xff0000) >> 16) * kernel[k];
					sum_g += (double)((pixel & 0x00ff00) >> 8) * kernel[k];
					sum_b += (double)(pixel & 0x0000ff) * kernel[k];
				}
			}
			row[x] = ((uint32_t)(sum_r + 0.5) << 16) |
				((uint32_t)(sum_g + 0.5) << 8) |
				((uint32_t)(sum_b + 0.5));
		}
		// Copy row back into pixels array
		for (x = 0; x < lW; x++) {
			pixels[(y + offY) * width + (x + offX)] = row[x];
		}
	}

	// Blur vertically
	for (x = 0; x < lW; x++) {
		for (y = 0; y < lH; y++) {
			int k;
			double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0;
			for (k = 0; k < kernel_size; k++) {
				int yk = y - (kernel_size - 1) / 2 + k;
				if (yk >= 0 && yk < lH) {
					uint32_t pixel = pixels[(yk + offY) * width + (x + offX)];
					sum_r += (double)((pixel & 0xff0000) >> 16) * kernel[k];
					sum_g += (double)((pixel & 0x00ff00) >> 8) * kernel[k];
					sum_b += (double)(pixel & 0x0000ff) * kernel[k];
				}
			}
			row[y] = ((uint32_t)(sum_r + 0.5) << 16) |
				((uint32_t)(sum_g + 0.5) << 8) |
				((uint32_t)(sum_b + 0.5));
		}
		// Copy row back into pixels array
		for (y = 0; y < lH; y++) {
			pixels[(offY + y) * width + (offX + x)] = row[y];
		}
	}

	// Free memory
	free(kernel);
	free(row);
}


#pragma endregion

