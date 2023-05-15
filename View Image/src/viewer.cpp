#define toolheight 43
#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <string>
#include "stb_image.h"
#include <string>
#include <stdint.h>
#include <stdbool.h>
#include "../resource.h"
#include "stb_image.h"
#include <immintrin.h> // for SIMD intrinsics
#pragma comment(lib, "shlwapi.lib")


int maxButtons = 9;
int selectedbutton = -1;

char filepath[1024];

bool standardFomat = true;


bool mouseDown = false;

POINT LockmPos;


float literalscaler = 1.0f;

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

std::string cpath;

bool active = false;

void* uiframe;
HDC hdc;
void* imgdata;
void* scrdata;
int imgwidth = 0;
int imgheight = 0;
int width = 1024;
int height = 576;


int ilocX = 0;
int ilocY = 0;

int lockimgoffx;
int lockimgoffy;

int CoordLeft;
int CoordTop;

int CoordRight;
int CoordBottom;

void RedrawImageOnBitmap(HWND hwnd);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stb_image.h"

#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS

#define GetMemoryLocation(start, x, y, widthfactor) \
	((uint32_t*)(start) + ((y) * (widthfactor)) + (x))\
\

bool fullscreen = false;

WINDOWPLACEMENT wpPrev;

const char* encodeimage(const char* filepath) {

	printf("\n -- Converting File -- \n");

	int imgwidth;
	int imgheight;
	int channels;
	void* imgdata = stbi_load(filepath, &imgwidth, &imgheight, &channels, 4);

	if (!imgdata) {
		return "sorry, file is not a bitmap or failed to load for some reason";
	}


	int imgByteSize = (imgwidth * imgheight * 4) + 2;

	void* data = malloc(imgByteSize);

	if (!data) {
		return "sorry, no image data";
	}

	if (imgwidth > 65536 || imgheight > 65536) {
		return "sorry, image width or height is too big";
	}


	wchar_t* ptr_ = (wchar_t*)data;

	*ptr_ = imgwidth;

	byte* ptr = (byte*)data;
	ptr += 2;


	//ptr += 4;

	//*ptr = height;

	//ptr += 4;


	for (int y = 0; y < imgheight; y++) {
		for (int x = 0; x < imgwidth; x++) {
			INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth));
			*ptr = pix;
			ptr++;
		}
	}


	for (int y = 0; y < imgheight; y++) {
		for (int x = 0; x < imgwidth; x++) {
			INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 8;
			*ptr = pix;
			ptr++;
		}
	}


	for (int y = 0; y < imgheight; y++) {
		for (int x = 0; x < imgwidth; x++) {
			INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 16;
			*ptr = pix;
			ptr++;
		}
	}

	for (int y = 0; y < imgheight; y++) {
		for (int x = 0; x < imgwidth; x++) {
			INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 24;
			*ptr = pix;
			ptr++;
		}
	}



	char str_path[256];
	strcpy(str_path, filepath);

	char* last_dot = strrchr(str_path, '.');
	if (last_dot != NULL) {
		*last_dot = '\0';
	}

	strcat(str_path, ".sfbb");

	// write to a file
	HANDLE hFile = CreateFile(
		str_path,     // Filename
		GENERIC_WRITE,          // Desired access
		FILE_SHARE_READ,        // Share mode
		NULL,                   // Security attributes
		CREATE_ALWAYS,             // Creates a new file, only if it doesn't already exist
		FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
		NULL);                  // Template file handle



	if (hFile == INVALID_HANDLE_VALUE)
	{
		return "sorry, no file handling";
	}


	// Write data to the file
	DWORD bytesWritten;
	WriteFile(
		hFile,            // Handle to the file
		data,  // Buffer to write
		imgByteSize,   // Buffer size
		&bytesWritten,    // Bytes written
		0);         // Overlapped

	// Close the handle once we don't need it.
	CloseHandle(hFile);

	free(imgdata);
	free(data);


	return 0;

}

void* decodesfbb(const char* filepath, int* imgwidth, int* imgheight) {
	printf("\n -- Reading File -- \n");
	HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		printf("sorry, the handle value was invalid");
		return 0;
	}

	DWORD fsize = GetFileSize(hFile, 0);
	printf("File Size: %i\n", fsize);

	int sizeOfAllocation = fsize;
	void* data = malloc(sizeOfAllocation);
	DWORD dwBytesRead = 0;
	DWORD dwBytesWritten = 0;

	if (!ReadFile(hFile, data, sizeOfAllocation, &dwBytesRead, NULL)) {
		return 0;
	}

	uint16_t* widthptr = (uint16_t*)data;
	if (widthptr == NULL) {
		printf("sorry, reading width failed\n");
		return 0;
	}


	int numOfPixels = (fsize - 2) / 4;

	printf("Number of pixels: %i\n", numOfPixels);

	int width = *(widthptr);

	printf("Width: %i\n", width);

	if (width == 0) {
		printf("Width is zero");
		return 0;
	}

	int height = numOfPixels / width;

	printf("Height: %i\n", height);

	int bitmapDataSize = numOfPixels * 4;

	void* bitmapData = malloc(bitmapDataSize);

	int* bmp_ptr = (int*)bitmapData;

	byte* dataptr = (byte*)data;

	for (int i = 0; i < (long long int)numOfPixels; i++) {

		// data
		byte* r_ptr = (dataptr + 2 + i);
		byte* g_ptr = (dataptr + 2 + i) + numOfPixels;
		byte* b_ptr = (dataptr + 2 + i) + numOfPixels * 2;
		byte* a_ptr = (dataptr + 2 + i) + numOfPixels * 3;

		//Gdiplus::Color c(*a_ptr, *r_ptr, *g_ptr, *b_ptr);

		int r = *r_ptr;
		int g = *g_ptr;
		int b = *b_ptr;
		int a = *a_ptr;

		int c = (a * 16777216) + (r * 65536) + (g * 256) + b;
		*bmp_ptr++ = c;
	}

	*imgwidth = width;
	*imgheight = height;

	return bitmapData;
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
	unsigned char* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(resourceData), resourceSize, &width, &height, &channels, 0);

	if (!imageData)
	{
		std::cerr << "Failed to load image from resource " << resourceId << ": " << stbi_failure_reason() << std::endl;
	}

	// Free the resource data
	FreeResource(hResourceData);

	return imageData;
}

std::string GetNextFilePath(const char* file_Path) {
	std::string imagePath = std::string(file_Path);
	std::string folderPath = imagePath.substr(0, imagePath.find_last_of("\\/"));
	std::string currentFileName = imagePath.substr(imagePath.find_last_of("\\/") + 1);
	bool foundCurrentFile = false;

	WIN32_FIND_DATAA fileData;
	HANDLE hFind;

	std::string searchPath = folderPath + "\\*.*";
	hFind = FindFirstFileA(searchPath.c_str(), &fileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::string fileName = fileData.cFileName;
				std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
				if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp" || extension == "gif")
				{
					if (foundCurrentFile)
					{
						std::wstring currentFileNameW = std::wstring(currentFileName.begin(), currentFileName.end());
						std::wstring fileNameW = std::wstring(fileName.begin(), fileName.end());
						if (StrCmpLogicalW(fileNameW.c_str(), currentFileNameW.c_str()) > 0)
						{
							std::string nextImagePath = folderPath + "\\" + fileName;
							FindClose(hFind);
							return nextImagePath;
						}
					}
					if (fileName == currentFileName)
					{
						foundCurrentFile = true;
					}
				}
			}
		} while (FindNextFileA(hFind, &fileData) != 0);

		FindClose(hFind);
	}

}

void no_offset(HWND hwnd) {
	ilocX = 0;
	ilocY = 0;

	RedrawImageOnBitmap(hwnd);
}

float log_base_1_25(float x) {
	float log_1_25 = log(1.25);
	return log(x) / log_1_25;
}

void autozoom(HWND hwnd) {

	no_offset(hwnd);

	float precentX = (float)width / (float)imgwidth;
	float precentY = (float)height / (float)imgheight;

	float e = fmin(precentX, precentY);

	float fzoom = e * 0.9f;
	// round to the nearest power of 1.25 (for easy zooming back to 100)
	literalscaler = pow(1.25f, round(log_base_1_25(fzoom)));

	if (literalscaler > 1.0f && imgheight > 5) literalscaler = 1.0f;

	RedrawImageOnBitmap(hwnd);
}

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

const int iconSize = 30; // 50px

bool OpenImage(HWND hwnd, const char* fpath) {
	strcpy(filepath, fpath);

	standardFomat = true;

	if (isFile(fpath, ".sfbb")) {

		standardFomat = false;

		imgdata = decodesfbb(fpath, &imgwidth, &imgheight);

		if (!imgdata) {
			MessageBox(hwnd, "Unable to load SFBB image", "Unknown Error", MB_OK);
			return false;
		}
	}
	else if(isFile(fpath, ".jpeg") || isFile(fpath, ".jpg") || isFile(fpath, ".png") || isFile(fpath, ".tga") || isFile(fpath, ".bmp") || isFile(fpath, ".psd") || isFile(fpath, ".gif") || isFile(fpath, ".hdr") || isFile(fpath, ".pic") || isFile(fpath, ".pmn")) {

		int channels;
		imgdata = stbi_load(fpath, &imgwidth, &imgheight, &channels, 4);


		if (!imgdata) {
			MessageBox(hwnd, "Unable to load primary image", "Unknown Error", MB_OK);
			return false;
		}
	}
	else {
		MessageBox(hwnd, "File type not supported", "Unsupported", MB_OK);
		return false;
	}
	

	// Auto-zoom
	autozoom(hwnd);

	cpath = std::string(fpath);

	RedrawImageOnBitmap(hwnd);
	return true;
}

void PrepareOpenImage(HWND hwnd);

float fLerp(float a, float b, float f) {
	return a * (1.0 - f) + (b * f);
}

char* path;

unsigned char* toolbarData;
int widthos, heightos, channelsos;

int GetButtonInterval() {
	return iconSize + 5;
}

int getXbuttonID(POINT mPos) {
	LONG x = mPos.x;
	int interval = GetButtonInterval();
	return mPos.y > toolheight ? -1 : x / interval;
}

//********************************************
// Here is where the program begins
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);
	path = (char*)malloc(size_needed);
	WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, path, size_needed, NULL, NULL);


	const char* CLASS_NAME = "ImageViewerClass";
	const char* WINDOW_NAME = "View Image";

	WNDCLASSEX wc = { 0 };
	
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.lpszClassName = CLASS_NAME;
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wc);

	RECT ws = { 0, 0, width, height };
	AdjustWindowRectEx(&ws, WS_OVERLAPPEDWINDOW, FALSE, NULL);
	int w_width = ws.right - ws.left;
	int w_height = ws.bottom - ws.top;

	uint32_t w = GetSystemMetrics(SM_CXSCREEN);
	uint32_t h = GetSystemMetrics(SM_CYSCREEN);

	uint32_t px = (w / 2) - (width / 2);
	uint32_t py = (h / 2) - (height / 2);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, px, py, w_width, w_height, NULL, NULL, NULL, NULL);

	scrdata = malloc(width*height * 4);

	toolbarData = LoadImageFromResource(IDB_PNG1, widthos, heightos, channelsos);

	hdc = GetDC(hwnd);


	if (argc >= 2) {
		if (!OpenImage(hwnd, path)) {
			MessageBox(hwnd, "Unable to open image", "Error", MB_OK);
			exit(0);
			return 0;
		}
	}
	else {
		RedrawImageOnBitmap(hwnd);
	}

	bool running = true;
	while (running) {

		HWND temp = GetActiveWindow();
		if (temp == hwnd) {

			if (GetKeyState('W') & 0x8000) {
				ilocY += 1;
				RedrawImageOnBitmap(hwnd);
			}
			if (GetKeyState('A') & 0x8000) {
				ilocX += 1;
				RedrawImageOnBitmap(hwnd);
			}
			if (GetKeyState('S') & 0x8000) {
				ilocY -= 1;
				RedrawImageOnBitmap(hwnd);
			}
			if (GetKeyState('D') & 0x8000) {
				ilocX -= 1;
				RedrawImageOnBitmap(hwnd);
			}
			/*
			if (fabs(literalscaler - scaler) > 0.003f) {
				scaler = fLerp(literalscaler, scaler, 0.9f);;
				RedrawImageOnBitmap(hwnd);
			}
			else {
				scaler = literalscaler;
			}
			*/

		}

		MSG msg = { 0 };
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT) { running = false; }
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	free(path);
	return 0;
}

const char* FileOpenDialog(HWND hwnd) {
	OPENFILENAME ofn = { 0 };
	TCHAR szFile[260] = { 0 };
	// Initialize remaining fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Supported Images (*.sfbb *.jpeg *.jpg *.png *.tga *.bmp *.psd; .gif; .hdr; .pic; .pnm)\0*.sfbb;*.jpeg;*.jpg;*.png;*.tga;*.bmp;*.psd;*.gif;*.hdr;*.pic;*.pnm\0SFBB (*.sfbb) only\0*.sfbb";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		return ofn.lpstrFile;
	}

	return 0;
}

uint32_t lerp(uint32_t c0, uint32_t c1, float amount) {
	uint8_t r1 = (c0 >> 16) & 0xff;
	uint8_t r2 = (c1 >> 16) & 0xff;
	uint8_t g1 = (c0 >> 8) & 0xff;
	uint8_t g2 = (c1 >> 8) & 0xff;
	uint8_t b1 = c0 & 0xff;
	uint8_t b2 = c1 & 0xff;
	return (uint32_t)((r2 - r1) * amount + r1) << 16 | (uint32_t)((g2 - g1) * amount + g1) << 8 | (uint32_t)((b2 - b1) * amount + b1);
}


uint32_t InvertColorChannels(uint32_t d) {
	if (!standardFomat) return d;
	return RGB((((d) >> 16) & 0xFF), (((d) >> 8) & 0xFF), (((d)) & 0xFF));
}
// Gaussian function
double gaussian(double x, double sigma) {
	return exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * 3.14159265) * sigma);
}


// Gaussian blur function
void gaussian_blur(uint32_t* pixels, int width, int height, double sigma) {
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
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int k;
			double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0;
			for (k = 0; k < kernel_size; k++) {
				int xk = x - (kernel_size - 1) / 2 + k;
				if (xk >= 0 && xk < width) {
					uint32_t pixel = pixels[y * width + xk];
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
		for (x = 0; x < width; x++) {
			pixels[y * width + x] = row[x];
		}
	}

	// Blur vertically
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			int k;
			double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0;
			for (k = 0; k < kernel_size; k++) {
				int yk = y - (kernel_size - 1) / 2 + k;
				if (yk >= 0 && yk < height) {
					uint32_t pixel = pixels[yk * width + x];
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
		for (y = 0; y < height; y++) {
			pixels[y * width + x] = row[y];
		}
	}

	// Free memory
	free(kernel);
	free(row);
}


uint32_t* bilinear_scale(const uint32_t* input_buffer, const int input_width, const int input_height,
	const float scale_factor, int& output_width, int& output_height) {

	// Calculate the output dimensions
	output_width = std::round(input_width * scale_factor);
	output_height = std::round(input_height * scale_factor);

	// Allocate memory for the output buffer
	uint32_t* output_buffer = new uint32_t[output_width * output_height];

	// Iterate over each pixel in the output buffer
	for (int y = 0; y < output_height; y++) {
		for (int x = 0; x < output_width; x++) {

			// Calculate the corresponding pixel in the input buffer
			float input_x = x / scale_factor;
			float input_y = y / scale_factor;

			// Calculate the four nearest pixels in the input buffer
			int x1 = std::floor(input_x);
			int x2 = std::ceil(input_x);
			int y1 = std::floor(input_y);
			int y2 = std::ceil(input_y);

			// Get the color of each of the four nearest pixels
			uint32_t c1 = input_buffer[y1 * input_width + x1];
			uint32_t c2 = input_buffer[y1 * input_width + x2];
			uint32_t c3 = input_buffer[y2 * input_width + x1];
			uint32_t c4 = input_buffer[y2 * input_width + x2];

			// Calculate the weights of each of the four nearest pixels
			float w1 = (x2 - input_x) * (y2 - input_y);
			float w2 = (input_x - x1) * (y2 - input_y);
			float w3 = (x2 - input_x) * (input_y - y1);
			float w4 = (input_x - x1) * (input_y - y1);

			// Calculate the final color of the output pixel
			uint32_t final_color = (uint32_t)(w1 * c1 + w2 * c2 + w3 * c3 + w4 * c4);

			// Set the output pixel color in the output buffer
			output_buffer[y * output_width + x] = final_color;
		}
	}

	// Return the output buffer
	return output_buffer;
}


//*********************************************
void RedrawImageOnBitmap(HWND hwnd) {

	// Clear the bitmap
	
	
	uint32_t color = 0x101010;

	bool paintBG = true;

	if (GetKeyState('L') & 0x8000 && GetKeyState('P') & 0x8000 && GetKeyState('M') & 0x8000) {
		paintBG = false;
	}
	
	for (uint32_t y = 0; y < height; y++) { 
		 
		for (uint32_t x = 0; x < width; x++) {
			uint32_t bkc{};
			// bkc
			if (((x / 10) + (y / 10)) % 2 == 0) {
				bkc = 0x161616;
			}
			else {
				bkc = 0x0C0C0C;
			}

			float nscaler = 1.0f / literalscaler;

			int32_t offX = (((int32_t)width - (int32_t)(imgwidth * literalscaler)) / 2);
			int32_t offY = (((int32_t)height + toolheight - (int32_t)(imgheight * literalscaler)) / 2);

			offX += ilocX;
			offY += ilocY;


			int32_t ptx = (x-offX) * nscaler;
			int32_t pty = (y-offY) * nscaler;

			
			int margin = 2;
			if (ptx < imgwidth && pty < imgheight && ptx >= 0 && pty >= 0 && x >= margin && y >= margin && y < height - margin && x < width - margin) {
				uint32_t c = *GetMemoryLocation(imgdata, ptx, pty, imgwidth);
				int blue = c & 255;
				int green = (c >> 8) & 25;
				int red = (c >> 16) & 255;
				int alpha = (c >> 24) & 255;

				*GetMemoryLocation(scrdata, x, y, width) = lerp(bkc, InvertColorChannels(c), ((float)alpha / 255.0f));

			}
			else {
				if (paintBG) {
					*GetMemoryLocation(scrdata, x, y, width) = bkc;
				}
			}

		}
	}


	if (imgwidth > 1) {

		CoordLeft = (width / 2) - (-ilocX)  - (int)(((float)(imgwidth / 2)) * literalscaler);
		CoordTop = (height / 2)  - (-ilocY) - (int)(((float)(imgheight / 2)) * literalscaler) + (toolheight/2);

		CoordRight = (width / 2) - (-ilocX) + (int)(((float)(imgwidth / 2)) * literalscaler);
		CoordBottom = (height / 2) - (-ilocY) + (int)(((float)(imgheight / 2)) * literalscaler) + (toolheight / 2);

	}


	if (!fullscreen) {

		// Render the toolbar

		if (height < 150) return;

		//BLUR FOR TOOLBAR
		if ((GetKeyState('N') & 0x8000) && (GetKeyState('M') & 0x8000)) {

			gaussian_blur((uint32_t*)scrdata, width, height, 4.0);
		}
		else if (CoordTop <= toolheight) {

			gaussian_blur((uint32_t*)scrdata, width, toolheight, 4.0);
		}
		else {

			for (uint32_t y = 0; y < toolheight; y++) {
				for (uint32_t x = 0; x < width; x++) {
					*GetMemoryLocation(scrdata, x, y, width) = 0x101010;
				}
			}
		}


		// TOOLBAR CONTAINER
		for (uint32_t y = 0; y < toolheight; y++) {
			for (uint32_t x = 0; x < width; x++) {
				uint32_t color = 0x050505;
				if (y == toolheight - 2) color = 0x303030;
				if (y == toolheight - 1) color = 0x000000;

				if (y == 0) color = 0x303030;
				if (y == 1) color = 0x000000;
				uint32_t* memoryPath = GetMemoryLocation(scrdata, x, y, width);
				*memoryPath = lerp(*memoryPath, color, 0.8f); // transparency
			}
		}

		// BUTTONS

		int p = 5;
		int k = 0;

		if (!toolbarData) return;


		for (int i = 0; i < maxButtons; i++) {
			float bop = 0.7f; // button opacity
			if (i == selectedbutton) bop = 1.0f;
			for (uint32_t y = 0; y < iconSize; y++) {
				for (uint32_t x = 0; x < iconSize; x++) {
					uint32_t l = (*GetMemoryLocation(toolbarData, x + k, y, widthos));
					float ll = (float)((l & 0x00FF0000) >> 16) / 255.0f;
					uint32_t* memoryPath = GetMemoryLocation(scrdata, p + x, 6 + y, width);
					*memoryPath = lerp(*memoryPath, 0xFFFFFF, ll * bop); // transparency
				}
			}
			p += iconSize + 5; k += iconSize + 1;
		}

		if (selectedbutton >= 0 && selectedbutton < maxButtons) {

			for (uint32_t y = 0; y < 1; y++) {
				for (uint32_t x = 0; x < GetButtonInterval() - 2; x++) {
					uint32_t* memoryPath = GetMemoryLocation(scrdata, x + 1 + (selectedbutton * GetButtonInterval() + 2), y + 4, width);
					*memoryPath = lerp(*memoryPath, 0xFFFFFF, 0.2f);
				}
			}
			for (uint32_t y = 0; y < toolheight - 10; y++) {
				for (uint32_t x = 0; x < GetButtonInterval(); x++) {
					uint32_t* memoryPath = GetMemoryLocation(scrdata, x + (selectedbutton * GetButtonInterval() + 2), y + 5, width);
					*memoryPath = lerp(*memoryPath, 0xFFFFFF, 0.2f);
				}
			}
			for (uint32_t y = 0; y < 1; y++) {
				for (uint32_t x = 0; x < GetButtonInterval() - 2; x++) {
					uint32_t* memoryPath = GetMemoryLocation(scrdata, x + 1 + (selectedbutton * GetButtonInterval() + 2), y + (toolheight - 5), width);
					*memoryPath = lerp(*memoryPath, 0xFFFFFF, 0.2f);
				}
			}
		}

	}
	
	// Update window title

	char str[256];
	if (!cpath.empty()) {

		sprintf(str, "View Image | %s | %d\%%", cpath.c_str(), (int)(literalscaler * 100.0f));
	}
	else {

		sprintf(str, "View Image | Type 'F' or click the folder icon to open new image");
	}

	SetWindowText(hwnd, str);
	
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -(int64_t)height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// tell our lovely win32 api to update the window for us <3
	SetDIBitsToDevice(hdc, 0, 0, width, height, 0, 0, 0, height, scrdata, &bmi, DIB_RGB_COLORS);
}

void PrepareOpenImage(HWND hwnd) {


	const char* path = FileOpenDialog(hwnd);

	if (path != 0) {

		OpenImage(hwnd, path);
	}

}

int SaveImage() {

	

	return 0;
}

int SaveSFBB() {

	int s = MessageBox(0, "Do you want to save this file as an SFBB? File will be saved in the current directory.", "Convert to SFBB?", MB_YESNO);
	if (s == IDYES) {
		if (imgwidth == 0) {
			MessageBox(0, "There is no image open", "No image", MB_OK);
			return 0;
		}
		if (!standardFomat) {
			MessageBox(0, "Image is not in a standard format or already in SFBB format.", "Already in SFBB Format", MB_OK);
			return 0;
		}
		const char* w = encodeimage(filepath);
		if (w == 0) {
			MessageBox(0, "Saved", "Saved", MB_OK);
		}
		else {
			MessageBox(0, w, "Save Failed", MB_OK);
		}
	}
	return 1;
}

void NewZoom(HWND hwnd, float v, bool mouse) {
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);
	p.x -= width / 2;
	p.y -= height / 2;

	int distance_x = ilocX - p.x;
	int distance_y = ilocY - p.y;
	int new_width = width * v;
	int new_height = height * v;
	if (mouse) {

		ilocX = p.x + distance_x * v;
		ilocY = p.y + distance_y * v;
	}
	literalscaler *= v;

	RedrawImageOnBitmap(hwnd);
}

bool lock = true;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_CREATE: {

		BOOL enable = TRUE;
		DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

		break;
	}

	case WM_LBUTTONDOWN:
	{
		POINT m;
		GetCursorPos(&m);
		ScreenToClient(hwnd, &m);
		uint32_t id = getXbuttonID(m);
		switch(id) {
		case 0:
			// open
			PrepareOpenImage(hwnd);
			break;
		case 1:
			// save
			if (!SaveSFBB()) {
				return 0;
			}
			break;
		case 2:
			// zoom in
			NewZoom(hwnd, 1.25f, false);
			break;
		case 3:
			// zoom out
			NewZoom(hwnd, 0.8f, false);
			break;
		case 4:
			// zoom fit
			autozoom(hwnd);
			break;
		case 5:
			// zoom original
			literalscaler = 1.0f;
			break;
		case 6:
			// rotate
			MessageBox(NULL, "I haven't added this feature yet", "Can't rotate image", MB_OK);
			break;
		case 7: {

			

			// delete
			int result = MessageBox(NULL, "This will delete the image permanently!!!", "Are You Sure?", MB_YESNO);
			if (result == IDYES) {
				DeleteFile(cpath.c_str());
				CHAR szPath[MAX_PATH];
				GetModuleFileName(NULL, szPath, MAX_PATH);

				// Start a new instance of the application
				ShellExecute(NULL, "open", szPath, NULL, NULL, SW_SHOWNORMAL);

				// Terminate the current instance of the application
				ExitProcess(0);
			}
			break;
		}
		case 8:
			// info
			MessageBox(NULL, filepath, "Image Info", MB_OK);
			break;
		}
	}
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:{
		lockimgoffx = ilocX;
		lockimgoffy = ilocY;
		GetCursorPos(&LockmPos);
		POINT k;
		GetCursorPos(&k);
		ScreenToClient(hwnd, &k);
		if (k.y > toolheight) {

			mouseDown = true;
		}
		break;
	}
	case WM_LBUTTONUP: {
	}
	case WM_MBUTTONUP:
	case WM_RBUTTONUP: {
		mouseDown = false;
		break;
	}
	case WM_MOUSELEAVE: {
	}
	case WM_MOUSEMOVE: {
		POINT pos = { 0 };
		GetCursorPos(&pos);
		if (mouseDown) {

			if (GetKeyState('G') & 0x8000) {
				ScreenToClient(hwnd, &pos);
				int k = (int)((float)(pos.x - CoordLeft) * (1.0f/literalscaler));
				int v = (int)((float)(pos.y - CoordTop) * (1.0f / literalscaler));
				
					for (int y = 0; y < 30; y++) {
						for (int x = 0; x < 30; x++) {
							bool yes = pow(x - 15, 2) + pow(y - 15, 2) < pow(15, 2);
							if (yes) {
								uint32_t xloc = (x - 15) + k;
								uint32_t yloc = (y - 15) + v;
								if (xloc < imgwidth && yloc < imgheight && xloc >= 0 && yloc >= 0) {
									*GetMemoryLocation(imgdata, xloc, yloc, imgwidth) = 0xFFFF0000;
								}
							}
						}
					}
				
				RedrawImageOnBitmap(hwnd);
				break;
			}
			else {

				ilocX = lockimgoffx - (LockmPos.x - pos.x);
				ilocY = lockimgoffy - (LockmPos.y - pos.y);
			}

			RedrawImageOnBitmap(hwnd);
		}

		ScreenToClient(hwnd, &pos);
		if (pos.y <= toolheight) {
			lock = true;
			selectedbutton = getXbuttonID(pos);
			RedrawImageOnBitmap(hwnd);
		}
		else {
			if (lock) {
				selectedbutton = -1;
				RedrawImageOnBitmap(hwnd);
				lock = false;
			}
		}
		break;
	}

	case WM_COMMAND: {
		if (wparam == 0x02) {
			Beep(400, 1000);
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	case WM_ACTIVATEAPP: {
		return 0;
	}
	case WM_KEYDOWN: {
		HWND temp = GetActiveWindow();
		if (temp != hwnd) {
			return 0;
		}
		if (wparam == VK_F11) {
			if (fullscreen) {
				// disable fullscreen
				SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

				SetWindowPlacement(hwnd, &wpPrev);
				SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
				fullscreen = false;
				RedrawImageOnBitmap(hwnd);
			}
			else {
				SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
				int screenX = GetSystemMetrics(SM_CXSCREEN);
				int screenY = GetSystemMetrics(SM_CYSCREEN);
				GetWindowPlacement(hwnd, &wpPrev);
				SetWindowPos(hwnd, 0, 0, 0, screenX, screenY, 0);
				fullscreen = true;
				RedrawImageOnBitmap(hwnd);
			}
		}
		if (wparam == VK_RIGHT) {
			/*
			const char* mpath = cpath.c_str();
			std::string k = GetNextFilePath(mpath);
			const char* npath = k.c_str();
			//MessageBox(0, mpath, npath, MB_OKCANCEL);
			if (npath != "" && npath != NULL) {
				OpenImage(hwnd, npath);
			}
			*/
		}
		if (wparam == 'Z') {
			if (!SaveSFBB()) {
				return 0;
			}
		}

		if (wparam == 'F') {
			PrepareOpenImage(hwnd);
		}

		if (wparam == '1') {
			literalscaler = 1.0f;
		}

		if (wparam == '2') {
			literalscaler = 2.0f;
		}
		if (wparam == '3') {
			literalscaler = 0.5f;
		}

		if (wparam == '4') {
			literalscaler = 4.0f;
		}

		if (wparam == '5') {
			autozoom(hwnd);
		}

		if (wparam == '6') {
			literalscaler = 0.25f;
		}

		if (wparam == '7') {
			literalscaler = 0.125f;
		}

		if (wparam == '8') {
			literalscaler = 8.0f;
		}
		RedrawImageOnBitmap(hwnd);
		break;
	}
	case WM_SIZE: {

		RECT ws = {0};
		GetClientRect(hwnd, &ws);
		width = ws.right - ws.left;
		height = ws.bottom - ws.top;


		free(scrdata);

		scrdata = malloc(width * height * 4);

		RedrawImageOnBitmap(hwnd);

		break;
	}
	case WM_MOUSEWHEEL: {
		if (imgwidth < 1) return 0;
		float zDelta = (float)GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

		float v = 1;

		if (zDelta > 0 && literalscaler < 400.0f) {
			v = 1.25f;
		}
		else if(literalscaler > 0.01f){
			v = 0.8f;
		}

		NewZoom(hwnd, v, true);

		break;
	}
	case WM_PAINT: {

		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -(int64_t)height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		SetDIBitsToDevice(hdc, 0, 0, width, height, 0, 0, 0, height, scrdata, &bmi, DIB_RGB_COLORS);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	default: {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	}
	return 0;
}
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")