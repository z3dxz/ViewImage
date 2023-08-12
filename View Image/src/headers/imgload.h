#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include "globalvar.h"

bool OpenImageFromPath(GlobalParams* m, std::string kpath);
void PrepareOpenImage(GlobalParams* m);
void PrepareSaveImage(GlobalParams* m);