#include "headers/ops.h"
#include "headers/imgload.h"
#include "../resource.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

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
	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), "PNG");
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

HDC GetPrinterDC() {
	PRINTDLG pd = { sizeof(PRINTDLG) };
	pd.Flags = PD_RETURNDC;

	if (PrintDlg(&pd)) {
		return pd.hDC;
	}

	return NULL;
}



void PrintImageToPrinter(uint32_t* image, int width, int height, HDC printerDC) {

	uint32_t* temp = (uint32_t*)malloc(width * height * 4);
	for (int i = 0; i < width * height; i++) {
		*(temp+i) = InvertColorChannels((*(image+i)), true);
	}

	if (!image) {
		// Handle loading error
		return;
	}

	BITMAPINFO bmpInfo = {};
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = width;
	bmpInfo.bmiHeader.biHeight = -height; // Negative height for top-down DIB
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 4 * 8; // Number of bits per pixel
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	// Start a print job
	DOCINFO docInfo = {};
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = TEXT("Image Print");
	StartDoc(printerDC, &docInfo);
	StartPage(printerDC);

	// Print the image to the printer
	StretchDIBits(printerDC, 0, 0, GetDeviceCaps(printerDC, HORZRES), (float)(GetDeviceCaps(printerDC, HORZRES)/width)*height,
		0, 0, width, height, temp, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);

	// End the print job
	EndPage(printerDC);
	EndDoc(printerDC);

	free(temp);

}

void Print(GlobalParams* m) {

	PRINTDLG pd = {};
	pd.lStructSize = sizeof(PRINTDLG);
	pd.Flags = PD_RETURNDC | PD_ALLPAGES;

	PrintDlg(&pd);

	// Get the printer device context
	HDC printerDC = pd.hDC;

	//CombineBuffer(m, (uint32_t*)m->imgdata, (uint32_t*)m->imgannotate, m->imgwidth, m->imgheight, true);
	PrintImageToPrinter((uint32_t*)m->imgdata, m->imgwidth, m->imgheight, printerDC);
	//FreeCombineBuffer(m);
}

void swapPointers(void*& ptr1, void*& ptr2) {
	void* temp = ptr1;
	ptr1 = ptr2;
	ptr2 = temp;
}
/*

void ResizeImageToSize(GlobalParams* m, int width, int height) {
	m->tempResizeBuffer = malloc(width * height * 4);
	stbir_resize_uint8((unsigned char*)m->imgdata, m->imgwidth, m->imgheight, 0, (unsigned char*)m->tempResizeBuffer, width, height, 0, 4);
	free(m->imgdata);
	m->imgwidth = width;
	m->imgheight = height;
	swapPointers(m->imgdata, m->tempResizeBuffer);
	m->shouldSaveShutdown = true;

	m->undoStep = 0;
	m->undoData.clear();
	free(m->imgannotate);
	m->imgannotate = malloc(width * height * 4);
	memset(m->imgannotate, 0x00, width * height * 4);
	autozoom(m);
	//free(to);
}
*/

void ResizeImageToSize(GlobalParams* m, int nwidth, int nheight) {


	createUndoStep(m);
	// allocate the copy for temporary reference
	void* tempOldBuffer = malloc(m->imgwidth * m->imgheight * 4);
	memcpy(tempOldBuffer, m->imgdata, m->imgwidth * m->imgheight * 4);

	int owidth = m->imgwidth;
	int oheight = m->imgheight;


	//CombineBuffer(m, (uint32_t*)m->imgdata, (uint32_t*)m->imgannotate, m->imgwidth, m->imgheight, true);
	// 
	// reallocate my image data boolean to suit the new data

	free(m->imgdata);
	m->imgdata = malloc(nwidth * nheight * 4);
	m->imgwidth = nwidth;
	m->imgheight = nheight;

	// do
	stbir_resize_uint8((unsigned char*)tempOldBuffer, owidth, oheight, 0, (unsigned char*)m->imgdata, nwidth, nheight, 0, 4);

	m->shouldSaveShutdown = true;

	//m->undoStep = 0;
	//m->undoData.clear();
	//free(m->imgannotate);
	//m->imgannotate = malloc(width * height * 4);
	//memset(m->imgannotate, 0x00, width * height * 4);
	autozoom(m);
	//FreeCombineBuffer(m);
	free(tempOldBuffer);
	RedrawSurface(m);




}


void rotateImage90Degrees(GlobalParams* m) {
	createUndoStep(m);
	// allocate the copy for tempoary reference
	void* tempOldBuffer = malloc(m->imgwidth * m->imgheight * 4);
	memcpy(tempOldBuffer, m->imgdata, m->imgwidth * m->imgheight * 4);
	
	int owidth = m->imgwidth;
	int oheight = m->imgheight;

	int nwidth = m->imgheight;
	int nheight = m->imgwidth; // reverse it

	//CombineBuffer(m, (uint32_t*)m->imgdata, (uint32_t*)m->imgannotate, m->imgwidth, m->imgheight, true);
	// 
	// reallocate my image data boolean to suit the new data

	free(m->imgdata);
	m->imgdata = malloc(nwidth * nheight * 4);
	m->imgwidth = nwidth;
	m->imgheight = nheight;

	// do
	for (size_t y = 0; y < nheight; y++) {
		for (size_t x = 0; x < nwidth; x++) {
			*GetMemoryLocation(m->imgdata, x, y, nwidth, nheight) = *GetMemoryLocation(tempOldBuffer, y, nwidth - 1 - x, owidth, oheight);
		}
	}
	
	m->shouldSaveShutdown = true;
	
	//m->undoStep = 0;
	//m->undoData.clear();
	//free(m->imgannotate);
	//m->imgannotate = malloc(width * height * 4);
	//memset(m->imgannotate, 0x00, width * height * 4);
	autozoom(m);
	//FreeCombineBuffer(m);
	free(tempOldBuffer);
	RedrawSurface(m);
	
}
/*

	//free(to);

	//
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
			*GetMemoryLocation(rotatedImage, x,y, newWidth, newHeight) = *GetMemoryLocation(m->imgdata,y, newWidth - 1 - x, m->imgheight, m->imgwidth);
		}
	}

	// Free memory of the original image
	free(m->imgdata);

	// Update pointer to point to rotated image
	m->imgdata = rotatedImage;
	m->shouldSaveShutdown = true;
	RedrawSurface(m);
}
*/

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
	if (!m->fullscreen) {
		m->iLocY = m->toolheight / 2;
	}
	else {
		m->iLocY = 0;
	}

	RedrawSurface(m);
}

void autozoom(GlobalParams* m) {

	int toolheight0 = 0;
	if (!m->fullscreen) { toolheight0 = m->toolheight; }

	no_offset(m);

	float precentX = (float)m->width / (float)m->imgwidth;
	float precentY = (float)(m->height - toolheight0) / (float)m->imgheight;

	float e = fmin(precentX, precentY);

	float fzoom = e;
	if (m->imgheight < 50) { fzoom = e / 2; }
	// round to the nearest power of 1.25 (for easy zooming back to 100)
	m->mscaler = fzoom;
	//if (mscaler > 1.0f && imgheight > 5) mscaler = 1.0f;

	RedrawSurface(m);
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

	RedrawSurface(m);
}


uint32_t InvertColorChannels(uint32_t d, bool should) {
	if (should) {
		return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
	}
	else {
		return d;
	}
}


double gammaCorrect(double value, double gamma) {
    return std::pow(value, 1.0 / gamma);
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




const int TABLE_SIZE = 256;

uint8_t  gamma_table[TABLE_SIZE];
uint8_t  inv_gamma_table[TABLE_SIZE];

void init_gamma_tables(float gamma) {
	for (int i = 0; i < TABLE_SIZE; ++i) {
		gamma_table[i] = (uint8_t)(std::pow(i / 255.0f, gamma)*255.0f);
		inv_gamma_table[i] = (uint8_t)(std::pow(i / 255.0f, 1.0f/gamma)*255.0f);
	}
}

bool yes = 0;
uint32_t lerp_gammacorrect(uint32_t color1, uint32_t color2, float alpha) {
	if (!yes) {
		init_gamma_tables(2.2);
		yes = 1;
	}

	// Extract the individual color channels from the input values and apply gamma correction using the lookup table
	uint8_t a1 = gamma_table[(color1 >> 24) & 0xFF];
	uint8_t r1 = gamma_table[(color1 >> 16) & 0xFF];
	uint8_t g1 = gamma_table[(color1 >> 8) & 0xFF];
	uint8_t b1 = gamma_table[color1 & 0xFF];

	uint8_t a2 = gamma_table[(color2 >> 24) & 0xFF];
	uint8_t r2 = gamma_table[(color2 >> 16) & 0xFF];
	uint8_t g2 = gamma_table[(color2 >> 8) & 0xFF];
	uint8_t b2 = gamma_table[color2 & 0xFF];

	// Calculate the lerped color values for each channel
	uint8_t a = (1 - alpha) * a1 + alpha * a2;
	uint8_t r = (1 - alpha) * r1 + alpha * r2;
	uint8_t g = (1 - alpha) * g1 + alpha * g2;
	uint8_t b = (1 - alpha) * b1 + alpha * b2;

	// Undo gamma correction

	
	a = inv_gamma_table[a];
	r = inv_gamma_table[r];
	g = inv_gamma_table[g];
	b = inv_gamma_table[b];
	

	// Combine the lerped color channels into a single 32-bit value
	return (a << 24) | (r << 16) | (g << 8) | b;
}


// Gaussian function
double gaussian(double x, double sigma) {
	return exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * 3.14159265) * sigma);
}

void boxBlur(uint32_t* mem, uint32_t width, uint32_t height, uint32_t kernelSize) {
	uint32_t halfKernel = kernelSize / 2;

	// Compute integral image
	uint32_t* integralR = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	uint32_t* integralG = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	uint32_t* integralB = (uint32_t*)malloc(width * height * sizeof(uint32_t));

	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t pixel = mem[y * width + x];
			uint32_t r = (pixel >> 16) & 0xFF;
			uint32_t g = (pixel >> 8) & 0xFF;
			uint32_t b = pixel & 0xFF;

			uint32_t sumR = r;
			uint32_t sumG = g;
			uint32_t sumB = b;

			if (x > 0) {
				sumR += integralR[y * width + x - 1];
				sumG += integralG[y * width + x - 1];
				sumB += integralB[y * width + x - 1];
			}
			if (y > 0) {
				sumR += integralR[(y - 1) * width + x];
				sumG += integralG[(y - 1) * width + x];
				sumB += integralB[(y - 1) * width + x];
			}
			if (x > 0 && y > 0) {
				sumR -= integralR[(y - 1) * width + x - 1];
				sumG -= integralG[(y - 1) * width + x - 1];
				sumB -= integralB[(y - 1) * width + x - 1];
			}

			integralR[y * width + x] = sumR;
			integralG[y * width + x] = sumG;
			integralB[y * width + x] = sumB;
		}
	}

	// Compute blurred image using integral images
	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t startX = x >= halfKernel ? x - halfKernel : 0;
			uint32_t startY = y >= halfKernel ? y - halfKernel : 0;
			uint32_t endX = x + halfKernel < width ? x + halfKernel : width - 1;
			uint32_t endY = y + halfKernel < height ? y + halfKernel : height - 1;

			uint32_t count = (endX - startX + 1) * (endY - startY + 1);
			uint32_t sumR = integralR[endY * width + endX];
			uint32_t sumG = integralG[endY * width + endX];
			uint32_t sumB = integralB[endY * width + endX];

			if (startX > 0) {
				sumR -= integralR[endY * width + startX - 1];
				sumG -= integralG[endY * width + startX - 1];
				sumB -= integralB[endY * width + startX - 1];
			}
			if (startY > 0) {
				sumR -= integralR[startY * width + endX];
				sumG -= integralG[startY * width + endX];
				sumB -= integralB[startY * width + endX];
			}
			if (startX > 0 && startY > 0) {
				sumR += integralR[startY * width + startX - 1];
				sumG += integralG[startY * width + startX - 1];
				sumB += integralB[startY * width + startX - 1];
			}

			uint32_t avgR = sumR / count;
			uint32_t avgG = sumG / count;
			uint32_t avgB = sumB / count;

			mem[y * width + x] = (avgR << 16) | (avgG << 8) | avgB;
		}
	}

	free(integralR);
	free(integralG);
	free(integralB);
}





/*
void gaussian_blur(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY) {
	unsigned char* result = (unsigned char*)malloc(width * height * 4);
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			for (int k = 0; k < 3; k++)
			{
				result[3 * row * width + 3 * col + k] = accessPixel((unsigned char*)pixels, col, row, k, width, height);
			}
		}
	}
	memcpy((unsigned char*)pixels, result, (width * height*4));
}

*/



uint32_t multiplyColor(uint32_t color, float multiplier) {
	// Extracting the individual color components
	uint8_t alpha = (color >> 24) & 0xFF;
	uint8_t red = (color >> 16) & 0xFF;
	uint8_t green = (color >> 8) & 0xFF;
	uint8_t blue = color & 0xFF;

	// Multiplying each color component by the multiplier
	red = static_cast<uint8_t>(red * multiplier);
	green = static_cast<uint8_t>(green * multiplier);
	blue = static_cast<uint8_t>(blue * multiplier);

	// Combining the color components back into a single color value
	return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

// Gaussian blur function
void gaussian_blur(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY) {
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

	// Allocate tempoary row array
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
					uint32_t pixel = *GetMemoryLocation(pixels, (xk + offX), (y + offY), width, height);
					//uint32_t pixel = pixels[(y + offY) * width + (xk + offX)];
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
			*GetMemoryLocation(pixels, (x + offX), (y + offY), width, height) = row[x];
			//pixels[(y + offY) * width + (x + offX)] = row[x];
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
					uint32_t pixel = *GetMemoryLocation(pixels, (x + offX), (yk + offY), width, height);
					//uint32_t pixel = pixels[(yk + offY) * width + (x + offX)];
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
			*GetMemoryLocation(pixels, (offX + x), (offY + y), width, height) = row[y];
			//pixels[(offY + y) * width + (offX + x)] = row[y];
		}
	}

	// Free memory
	free(kernel);
	free(row);
}


#pragma endregion


uint8_t clamp(int value) {
	// Clamp the value to the range [0, 255]
	return (uint8_t)(value < 0 ? 0 : (value > 255 ? 255 : value));
}

#include <stdint.h>

void gaussian_blur_toolbar(GlobalParams* m, uint32_t* pixels) {


	unsigned char* px = (unsigned char*)pixels;
	unsigned char* gd = (unsigned char*)m->toolbar_gaussian_data;

	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	//fast_gaussian_blur(px, gd, m->width, m->toolheight, 4, 4.0f, 10, Border::kKernelCrop);
	//gaussian_blur(pixels, m->width, 40, 4.0, m->width, 40, 0, 0);
	boxBlur(pixels, m->width, 40, 15);

	//convolution((uint32_t*)m->scrdata, (uint32_t*)m->toolbar_gaussian_data, m->width, 40, kernel, 9);

	std::chrono::steady_clock::time_point end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> duration = end - start;
	std::string k0 = "Execution time: " + std::to_string(duration.count()) + " seconds.";


	// it swap itself
	//std::swap(px, gd);

}


double remap(double value, double fromLow, double fromHigh, double toLow, double toHigh) {
	// Check for invalid input ranges
	if (fromLow == fromHigh) {
		std::cerr << "Error: 'from' range is degenerate (fromLow == fromHigh)" << std::endl;
		return value;  // Return the input value unchanged
	}

	// Map the value from the 'from' range to the 'to' range
	double result = toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);

	// Clamp the result to the 'to' range
	result = min(max(result, toLow), toHigh);

	return result;
}