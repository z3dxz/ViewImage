#pragma once
#include "ops.hpp"

void clear_kvector();
std::string GetPrevFilePath();
std::string GetNextFilePath(const char* file_Path);


void GoLeft(GlobalParams* m);

void GoRight(GlobalParams* m);