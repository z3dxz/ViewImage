#pragma once
#include <windows.h>

const char* encodeimage(const char* filepath);
void* decodesfbb(const char* filepath, int* imgwidth, int* imgheight);