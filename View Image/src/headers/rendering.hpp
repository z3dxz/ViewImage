#pragma once
#include "ops.hpp"
#include "globalvar.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <execution>
#include <ft2build.h>
#include <freetype/freetype.h>

#pragma comment(lib, "freetype.lib")

void ResetCoordinates(GlobalParams* m);
bool InitFont(GlobalParams* m, std::string fontA, int size);
void RedrawSurface(GlobalParams* m);
void CircleGenerator(int circleDiameter, int locX, int locY, int outlineThickness, int smoothening, uint32_t* backgroundBuffer, uint32_t foregroundColor, float opacity, int backgroundWidth, int backgroundHeight);