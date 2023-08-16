#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include "globalvar.h"

bool doIFSave(GlobalParams* m);
bool OpenImageFromPath(GlobalParams* m, std::string kpath, bool isLeftRight);
void PrepareOpenImage(GlobalParams* m);
void PrepareSaveImage(GlobalParams* m);