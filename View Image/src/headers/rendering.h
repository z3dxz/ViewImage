#pragma once
#include "ops.h"
#include "globalvar.h"
#include <freetype/freetype.h>

bool InitFont(HWND hwnd, const char* font, int size);
void RedrawImageOnBitmap(GlobalParams* m);