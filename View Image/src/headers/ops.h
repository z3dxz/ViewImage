#pragma once
#include "globalvar.h"
#include <Windows.h>
#include "rendering.h"
#include <iostream>
#include "../stb_image.h"
#include "../stb_image_write.h"

#pragma region Memory

#define GetMemoryLocation(start, x, y, widthfactor) \
	((uint32_t*)(start) + ((y) * (widthfactor)) + (x))\
\

#pragma endregion

#pragma region File

bool isFile(const char* str, const char* suffix);

unsigned char* LoadImageFromResource(int resourceId, int& width, int& height, int& channels);
#pragma endregion

#pragma region Math


void rotateImage90Degrees(GlobalParams* m);

int GetButtonInterval(GlobalParams* m);

int getXbuttonID(GlobalParams* m, POINT mPos);
float log_base_1_25(float x);

float roundzoom(float z);

void no_offset(GlobalParams* m);

void autozoom(GlobalParams* m);


void NewZoom(GlobalParams* m, float v, int mouse);

uint32_t InvertColorChannelsInverse(uint32_t d);
uint32_t InvertColorChannels(uint32_t d);
uint32_t lerp(uint32_t color1, uint32_t color2, float alpha);



// Gaussian function
double gaussian(double x, double sigma);


// Gaussian blur function
void gaussian_blur(uint32_t* pixels, int lW, int lH, double sigma, uint32_t width, uint32_t offX = 0, uint32_t offY = 0);

#pragma endregion
