#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "../headers/ops.hpp"
#include <string.h>
#include "../stb_image.h"

#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS

#define GetMemoryLocation(start, x, y, widthfactor) \
	((uint32_t*)start + (y * widthfactor) + x)\
\

const char* encodeimage(const char* filepath) {

    printf("\n -- Converting File -- \n");

    int imgwidth;
    int imgheight;
    int channels;
    void* imgdata = stbi_load(filepath, &imgwidth, &imgheight, &channels, 4);

    if (!imgdata) {
        return "sorry, file is not a bitmap or failed to load for some reason";
    }


    int imgByteSize = (imgwidth * imgheight * 4) + 2;

    void* data = malloc(imgByteSize);

    if (!data) {
        return "sorry, no image data";
    }

    if (imgwidth > 65536 || imgheight > 65536) {
        return "sorry, image width or height is too big";
    }


    wchar_t* ptr_ = (wchar_t*)data;

    *ptr_ = imgwidth;

    byte* ptr = (byte*)data;
    ptr += 2;


    //ptr += 4;

    //*ptr = height;

    //ptr += 4;


    for (int y = 0; y < imgheight; y++) {
        for (int x = 0; x < imgwidth; x++) {
            INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth));
            *ptr = pix;
            ptr++;
        }
    }


    for (int y = 0; y < imgheight; y++) {
        for (int x = 0; x < imgwidth; x++) {
            INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 8;
            *ptr = pix;
            ptr++;
        }
    }


    for (int y = 0; y < imgheight; y++) {
        for (int x = 0; x < imgwidth; x++) {
            INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 16;
            *ptr = pix;
            ptr++;
        }
    }

    for (int y = 0; y < imgheight; y++) {
        for (int x = 0; x < imgwidth; x++) {
            INT8 pix = (*GetMemoryLocation(imgdata, x, y, imgwidth)) >> 24;
            *ptr = pix;
            ptr++;
        }
    }



    char str_path[256];
    strcpy(str_path, filepath);

    char* last_dot = strrchr(str_path, '.');
    if (last_dot != NULL) {
        *last_dot = '\0';
    }

    strcat(str_path, ".sfbb");

    // write to a file
    HANDLE hFile = CreateFile(
        str_path,     // Filename
        GENERIC_WRITE,          // Desired access
        FILE_SHARE_READ,        // Share mode
        NULL,                   // Security attributes
        CREATE_ALWAYS,             // Creates a new file, only if it doesn't already exist
        FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
        NULL);                  // Template file handle



    if (hFile == INVALID_HANDLE_VALUE)
    {
        return "sorry, no file handling";
    }


    // Write data to the file
    DWORD bytesWritten;
    WriteFile(
        hFile,            // Handle to the file
        data,  // Buffer to write
        imgByteSize,   // Buffer size
        &bytesWritten,    // Bytes written
        0);         // Overlapped

    // Close the handle once we don't need it.
    CloseHandle(hFile);

    FreeData(imgdata);
    FreeData(data);

    return 0;

}

void* decodesfbb(const char* filepath, int* imgwidth, int* imgheight) {
    printf("\n -- Reading File -- \n");
    HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("sorry, the handle value was invalid");
        return 0;
    }

    DWORD fsize = GetFileSize(hFile, 0);
    printf("File Size: %i\n", fsize);
    
    int sizeOfAllocation = fsize;
    void* data = malloc(sizeOfAllocation);
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;

    if (!ReadFile(hFile, data, sizeOfAllocation, &dwBytesRead, NULL)) {
        return 0;
    }

    uint16_t* widthptr = (uint16_t*)data;
    if (widthptr == NULL) {
        printf("sorry, reading width failed\n");
        return 0;
    }


    int numOfPixels = (fsize - 2) / 4;

    printf("Number of pixels: %i\n", numOfPixels);

    int width = *(widthptr);

    printf("Width: %i\n", width);

    if (width == 0) {
        printf("Width is zero");
        return 0;
    }

    int height = numOfPixels / width;

    printf("Height: %i\n", height);

    int bitmapDataSize = numOfPixels * 4;

    void* bitmapData = malloc(bitmapDataSize);

    int* bmp_ptr = (int*)bitmapData;

    byte* dataptr = (byte*)data;

    for (int i = 0; i < (long long int)numOfPixels; i++) {

        // data
        byte* r_ptr = (dataptr + 2 + i);
        byte* g_ptr = (dataptr + 2 + i) + numOfPixels;
        byte* b_ptr = (dataptr + 2 + i) + numOfPixels * 2;
        byte* a_ptr = (dataptr + 2 + i) + numOfPixels * 3;

        //Gdiplus::Color c(*a_ptr, *r_ptr, *g_ptr, *b_ptr);

        int r = *r_ptr;
        int g = *g_ptr;
        int b = *b_ptr;
        int a = *a_ptr;

        int c = (a * 16777216) + (r * 65536) + (g * 256) + b;
        *bmp_ptr++ = c;
    }

    *imgwidth = width;
    *imgheight = height;

    return bitmapData;
}