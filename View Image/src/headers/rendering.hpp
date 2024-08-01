#pragma once
#include "ops.hpp"
#include "globalvar.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <execution>

#pragma comment(lib, "freetype.lib")

void ResetCoordinates(GlobalParams* m);
//bool InitFont(GlobalParams* m, std::string fontA, int size);

FT_Face LoadFont(GlobalParams* m, std::string fontA);

void UpdateBuffer(GlobalParams* m);
void RedrawSurface(GlobalParams* m, bool onlyImage = false, bool doesManualClip = false);
void CircleGenerator(int circleDiameter, int locX, int locY, int outlineThickness, int smoothening, uint32_t* backgroundBuffer, uint32_t foregroundColor, float opacity, int backgroundWidth, int backgroundHeight);

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem);
int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer);

void SwitchFont(FT_Face& font);