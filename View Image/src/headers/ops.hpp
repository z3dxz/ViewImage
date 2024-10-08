#pragma once
#include <iostream>
#include <algorithm>
#include "globalvar.hpp"
#include <Windows.h>
#include "rendering.hpp"
#include "events.hpp"
#include "../stb_image.h"
#include "../stb_image_write.h"

inline void FreeDatac(void*& b) {
	if (b) {
		free(b);
		b = nullptr;
	}
}
#define FreeData(x) \
	FreeDatac((void*&)x)
		

bool DwmDarken(HWND hwnd);

void DeleteTempFiles(GlobalParams* m);
bool DeleteDirectory(const char* directoryPath);
#pragma region Memory


#define IfInMenu(pos, m) \
	((pos.x > m->actmenuX && pos.y > m->actmenuY && pos.x < (m->actmenuX + m->menuSX) && pos.y < (m->actmenuY + m->menuSY)))

#define IsInImage(mPP, m) \
	(mPP.y > m->toolheight && mPP.x >= m->CoordLeft && mPP.y > m->CoordTop && mPP.x < m->CoordRight && mPP.y < m->CoordBottom)

#define CheckIfMouseInSlider1(mPP, m, slider1begin, slider1end, sliderYb, sliderYe) \
	((mPP.x > slider1begin && mPP.x < slider1end) && (mPP.y > sliderYb && mPP.y < sliderYe))

#define CheckIfMouseInSlider2(mPP, m, slider2begin, slider2end, sliderYb, sliderYe) \
	((mPP.x > slider2begin && mPP.x < slider2end) && (mPP.y > sliderYb && mPP.y < sliderYe))

double remap(double value, double fromLow, double fromHigh, double toLow, double toHigh);
#define GetMemoryLocation(start, x, y, widthfactor, heightfactor) \
	 (( (((y) * (widthfactor)) + (x)) < (widthfactor*heightfactor) &&      ( (((y)*(widthfactor)) + (x)) > 0  )            ) ? ((uint32_t*)(start) + ((y) * (widthfactor)) + (x))  : ((uint32_t*)(start)) ) 

#define GetMemoryLocationTemplate(start, x, y, widthfactor, heightfactor) \
	 ((( (((y) * (widthfactor)) + (x)) < (widthfactor*heightfactor))&&((y) * (widthfactor)) + (x) > start) ) ? ((start) + ((y) * (widthfactor)) + (x))  : ((start)) ) 

#pragma endregion

#pragma region File

bool isFile(const char* str, const char* suffix);

unsigned char* LoadImageFromResource(int resourceId, int& width, int& height, int& channels);
#pragma endregion

#pragma region Math


void Print(GlobalParams* m);
void rotateImage90Degrees(GlobalParams* m);

int GetLocationFromButton(GlobalParams* m, int index);
int GetIndividualButtonPush(GlobalParams* m, int index);
int getXbuttonID(GlobalParams* m, POINT mPos);

void GetCropCoordinates(GlobalParams* m, uint32_t* outDistLeft, uint32_t* outDistRight, uint32_t* outDistTop, uint32_t* outDistBottom);
void GetCropPercentagesFromCursor(GlobalParams* m, int cursorX, int cursorY, float* outX, float* outY);

void ConfirmCrop(GlobalParams* m);

float log_base_1_25(float x);

float roundzoom(float z);

void no_offset(GlobalParams* m);

void autozoom(GlobalParams* m);

uint8_t getAlpha(uint32_t color);

void NewZoom(GlobalParams* m, float v, int mouse, bool shouldRoundZoom);

uint32_t InvertCC(uint32_t d, bool should);
void InvertAllColorChannels(uint32_t* buffer, int w, int h);

uint32_t lerp(uint32_t color1, uint32_t color2, float alpha);
uint8_t lerpLinear(uint8_t a, uint8_t b, float t);
uint32_t lerp_gc(uint32_t color1, uint32_t color2, float alpha);

void ResizeImageToSize(GlobalParams* m, int width, int height);

// Gaussian function
double gaussian(double x, double sigma);

void boxBlur(uint32_t* mem, uint32_t width, uint32_t height, uint32_t kernelSize, int mode);
// Gaussian blur function
void gaussian_blur(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX = 0, uint32_t offY = 0);

void gaussian_blur_toolbar(GlobalParams* m, uint32_t* pixels);
#pragma endregion

void gaussian_blur_B(uint32_t* input_buffer, uint32_t* output_buffer, int lW, int lH, double sigma, uint32_t width, uint32_t height, uint32_t offX, uint32_t offY);
void gaussianBlurFFT(uint32_t* image, int height, int width, int kernelSize, double* kernel);

uint32_t multiplyColor(uint32_t color, float multiplier);


uint32_t subtractColors(uint32_t color1, uint32_t color2);
bool CopyImageToClipboard(GlobalParams* m, void* imageData, int width, int height);
bool PasteImageFromClipboard(GlobalParams* m);

uint32_t change_alpha(uint32_t color, uint8_t new_alpha);
void overlayBuffers(const uint32_t* background, const uint32_t* foreground, uint32_t* output, float opacity, size_t bufferSize);

void AutoAdjustLevels(GlobalParams* m, uint32_t* buffer);