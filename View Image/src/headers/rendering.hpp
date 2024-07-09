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
void RedrawSurface(GlobalParams* m, bool onlyImage = false);
void CircleGenerator(int circleDiameter, int locX, int locY, int outlineThickness, int smoothening, uint32_t* backgroundBuffer, uint32_t foregroundColor, float opacity, int backgroundWidth, int backgroundHeight);