#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include "globalvar.h"


//void CombineBuffer(GlobalParams* m, uint32_t* first, uint32_t* second, int width, int height, bool invert);
//void FreeCombineBuffer(GlobalParams* m);
bool doIFSave(GlobalParams* m);
bool OpenImageFromPath(GlobalParams* m, std::string kpath, bool isLeftRight);
void PrepareOpenImage(GlobalParams* m);
void PrepareSaveImage(GlobalParams* m);
bool AllocateBlankImage(GlobalParams* m, uint32_t color);