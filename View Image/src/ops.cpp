#include "headers/ops.h"
#include "../resource.h"


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
		*(temp+i) = InvertColorChannels((*(image+i)));
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
	PrintImageToPrinter((uint32_t*)m->imgdata, m->imgwidth, m->imgheight, printerDC);
}

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

uint32_t InvertColorChannelsInverse(uint32_t d) {
	return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
}

uint32_t InvertColorChannels(uint32_t d) {
	return (d & 0xFF00FF00) | ((d & 0x00FF0000) >> 16) | ((d & 0x000000FF) << 16);
}

static const float gammaLUTL[256] = { 0, 0.00129465, 0.00594864, 0.014515, 0.0273328, 0.0446566, 0.0666936, 0.0936197, 0.125588, 0.162737, 0.205188, 0.253055, 0.306444, 0.365449, 0.430163, 0.500671, 0.577053, 0.659386, 0.747741, 0.842189, 0.942796, 1.04963, 1.16274, 1.28219, 1.40804, 1.54035, 1.67916, 1.82453, 1.97651, 2.13514, 2.30048, 2.47256, 2.65144, 2.83715, 3.02974, 3.22925, 3.43572, 3.64918, 3.86969, 4.09726, 4.33195, 4.57379, 4.82281, 5.07905, 5.34254, 5.61331, 5.89141, 6.17685, 6.46968, 6.76991, 7.0776, 7.39275, 7.71541, 8.0456, 8.38336, 8.7287, 9.08166, 9.44227, 9.81055, 10.1865, 10.5702, 10.9617, 11.3609, 11.768, 12.1828, 12.6055, 13.0361, 13.4746, 13.921, 14.3754, 14.8377, 15.3081, 15.7864, 16.2728, 16.7672, 17.2698, 17.7804, 18.2992, 18.8261, 19.3612, 19.9044, 20.4559, 21.0156, 21.5836, 22.1598, 22.7443, 23.3372, 23.9383, 24.5479, 25.1657, 25.792, 26.4267, 27.0698, 27.7213, 28.3814, 29.0498, 29.7268, 30.4123, 31.1064, 31.8089, 32.5201, 33.2398, 33.9682, 34.7051, 35.4507, 36.205, 36.9679, 37.7395, 38.5198, 39.3088, 40.1066, 40.9131, 41.7284, 42.5524, 43.3853, 44.227, 45.0775, 45.9368, 46.805, 47.6821, 48.568, 49.4629, 50.3667, 51.2794, 52.2011, 53.1317, 54.0713, 55.0199, 55.9775, 56.9442, 57.9198, 58.9045, 59.8983, 60.9011, 61.9131, 62.9341, 63.9643, 65.0035, 66.052, 67.1096, 68.1763, 69.2523, 70.3374, 71.4317, 72.5353, 73.6481, 74.7701, 75.9014, 77.042, 78.1919, 79.351, 80.5195, 81.6973, 82.8844, 84.0809, 85.2867, 86.502, 87.7265, 88.9605, 90.2039, 91.4568, 92.719, 93.9907, 95.2718, 96.5624, 97.8625, 99.1721, 100.491, 101.82, 103.158, 104.506, 105.863, 107.23, 108.606, 109.992, 111.387, 112.792, 114.207, 115.631, 117.065, 118.509, 119.962, 121.425, 122.898, 124.38, 125.872, 127.374, 128.885, 130.406, 131.937, 133.478, 135.028, 136.589, 138.159, 139.738, 141.328, 142.927, 144.536, 146.156, 147.784, 149.423, 151.072, 152.73, 154.398, 156.077, 157.765, 159.463, 161.171, 162.889, 164.617, 166.354, 168.102, 169.86, 171.628, 173.405, 175.193, 176.991, 178.798, 180.616, 182.444, 184.281, 186.129, 187.987, 189.855, 191.733, 193.621, 195.52, 197.428, 199.346, 201.275, 203.214, 205.163, 207.122, 209.091, 211.07, 213.06, 215.059, 217.069, 219.089, 221.12, 223.16, 225.211, 227.272, 229.343, 231.425, 233.516, 235.618, 237.731, 239.853, 241.986, 244.129, 246.283, 248.447, 250.621, 252.805, 255 };
static const float gammaLUTS[256] = { 0, 21.0667, 28.778, 34.5384, 39.3119, 43.4644, 47.1808, 50.5698, 53.7016, 56.6247, 59.3741, 61.976, 64.4509, 66.8146, 69.0804, 71.2587, 73.3586, 75.3875, 77.3517, 79.2567, 81.1074, 82.9079, 84.6618, 86.3723, 88.0425, 89.6747, 91.2715, 92.8348, 94.3666, 95.8686, 97.3423, 98.7893, 100.211, 101.608, 102.982, 104.334, 105.666, 106.976, 108.268, 109.541, 110.796, 112.034, 113.256, 114.461, 115.651, 116.827, 117.988, 119.135, 120.27, 121.391, 122.499, 123.596, 124.681, 125.754, 126.816, 127.868, 128.909, 129.939, 130.96, 131.972, 132.974, 133.966, 134.95, 135.925, 136.892, 137.85, 138.801, 139.743, 140.678, 141.605, 142.525, 143.438, 144.343, 145.242, 146.134, 147.019, 147.898, 148.771, 149.637, 150.498, 151.352, 152.2, 153.043, 153.88, 154.712, 155.538, 156.358, 157.174, 157.984, 158.79, 159.59, 160.386, 161.176, 161.962, 162.744, 163.521, 164.293, 165.061, 165.825, 166.584, 167.339, 168.09, 168.837, 169.58, 170.319, 171.054, 171.785, 172.512, 173.236, 173.956, 174.672, 175.385, 176.094, 176.8, 177.502, 178.201, 178.897, 179.589, 180.279, 180.964, 181.647, 182.327, 183.003, 183.677, 184.347, 185.015, 185.679, 186.341, 187, 187.656, 188.309, 188.96, 189.607, 190.252, 190.895, 191.535, 192.172, 192.806, 193.438, 194.068, 194.695, 195.32, 195.942, 196.561, 197.179, 197.794, 198.407, 199.017, 199.625, 200.231, 200.834, 201.436, 202.035, 202.632, 203.227, 203.82, 204.41, 204.999, 205.586, 206.17, 206.753, 207.333, 207.912, 208.488, 209.063, 209.636, 210.206, 210.775, 211.342, 211.907, 212.471, 213.032, 213.592, 214.15, 214.706, 215.26, 215.813, 216.364, 216.913, 217.461, 218.007, 218.551, 219.093, 219.634, 220.174, 220.711, 221.247, 221.782, 222.315, 222.846, 223.376, 223.904, 224.431, 224.956, 225.48, 226.002, 226.523, 227.042, 227.56, 228.077, 228.592, 229.105, 229.618, 230.128, 230.638, 231.146, 231.653, 232.158, 232.662, 233.165, 233.666, 234.166, 234.665, 235.162, 235.659, 236.154, 236.647, 237.14, 237.631, 238.121, 238.609, 239.097, 239.583, 240.068, 240.552, 241.035, 241.516, 241.996, 242.475, 242.953, 243.43, 243.906, 244.381, 244.854, 245.326, 245.798, 246.268, 246.737, 247.205, 247.672, 248.137, 248.602, 249.066, 249.528, 249.99, 250.45, 250.91, 251.368, 251.826, 252.282, 252.738, 253.192, 253.646, 254.098, 254.55, 255 };

uint32_t lerpz(uint32_t color1, uint32_t color2, float alpha)
{
	// Extract the individual color channels from the input values
	float a1 = gammaLUTL[(color1 >> 24) & 0xFF];
	float r1 = gammaLUTL[(color1 >> 16) & 0xFF];
	float g1 = gammaLUTL[(color1 >> 8) & 0xFF];
	float b1 = gammaLUTL[color1 & 0xFF];

	float a2 = gammaLUTL[(color2 >> 24) & 0xFF];
	float r2 = gammaLUTL[(color2 >> 16) & 0xFF];
	float g2 = gammaLUTL[(color2 >> 8) & 0xFF];
	float b2 = gammaLUTL[color2 & 0xFF];

	// Calculate the lerped color values for each channel
	uint8_t a = gammaLUTS[(uint8_t)((1.0f - alpha) * a1 + alpha * a2)];
	uint8_t r = gammaLUTS[(uint8_t)((1.0f - alpha) * r1 + alpha * r2)];
	uint8_t g = gammaLUTS[(uint8_t)((1.0f - alpha) * g1 + alpha * g2)];
	uint8_t b = gammaLUTS[(uint8_t)((1.0f - alpha) * b1 + alpha * b2)];

	// Combine the lerped color channels into a single 32-bit value
	return (a << 24) | (r << 16) | (g << 8) | b;
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



int kernel[3][3] = { 1, 2, 1,
				   2, 4, 2,
				   1, 2, 1 };
/*
int accessPixel(unsigned char* arr, int col, int row, int k, int width, int height)
{
	int sum = 0;
	int sumKernel = 0;

	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			if ((row + j) >= 0 && (row + j) < height && (col + i) >= 0 && (col + i) < width)
			{
				int color = arr[(row + j) * 3 * width + (col + i) * 3 + k];
				sum += color * kernel[i + 1][j + 1];
				sumKernel += kernel[i + 1][j + 1];
			}
		}
	}

	return sum / sumKernel;
}

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

void gaussian_blur_f(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY) {

	int size = width * height;
	unsigned char* result = (unsigned char*)malloc(size * 4);
	unsigned char* px = (unsigned char*)pixels;

	fast_gaussian_blur(px, result, width, lH, 4, 4.0f, 10, Border::kKernelCrop);

	// it swap itself
	delete[] px;
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

