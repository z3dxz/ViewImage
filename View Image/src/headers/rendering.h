#pragma once
#include "ops.h"
#include "globalvar.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <execution>
#include <ft2build.h>
#include <freetype/freetype.h>

#pragma comment(lib, "freetype.lib")

bool InitFont(GlobalParams* m, const char* font, int size);
void RedrawSurface(GlobalParams* m);