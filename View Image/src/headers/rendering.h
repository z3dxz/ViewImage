#pragma once
#include "ops.h"
#include "globalvar.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <execution>
#include <freetype/freetype.h>

bool InitFont(HWND hwnd, const char* font, int size);
void RedrawImageOnBitmap(GlobalParams* m);