
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <string>
#include "../resource.h"
#include <cstdint>
#include <dwmapi.h>
#include "headers/globalvar.h"
#include "headers/imgload.h"
#include "headers/events.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
// draw vars

GlobalParams gp;


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {


	// get argument info
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	const char* CLASS_NAME = "ImageViewerClass";
	const char* WINDOW_NAME = "View Image (Loading)";

	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.lpszClassName = CLASS_NAME;
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wc);

	RECT ws = { 0, 0, gp.width, gp.height };
	AdjustWindowRectEx(&ws, WS_OVERLAPPEDWINDOW, FALSE, NULL);
	int w_width = ws.right - ws.left;
	int w_height = ws.bottom - ws.top;

	uint32_t w = GetSystemMetrics(SM_CXSCREEN);
	uint32_t h = GetSystemMetrics(SM_CYSCREEN);

	uint32_t px = (w / 2) - (gp.width / 2);
	uint32_t py = (h / 2) - (gp.height / 2);


	// RIGHT HERE: WIDTH = 1024

	gp.hwnd = CreateWindow(CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, px, py, w_width, w_height, NULL, NULL, NULL, NULL);


	// RIGHT HERE: WIDTH = 0

	gp.hdc = GetDC(gp.hwnd);

	if (!Initialization(&gp, argc, argv)) {
		return 0;
	}


	bool running = true;
	while (running) {

		HWND temp = GetActiveWindow();
		if (temp == gp.hwnd) {

			if (GetKeyState('W') & 0x8000) {
				gp.iLocY += 3;
				RedrawImageOnBitmap(&gp);
			}
			if (GetKeyState('A') & 0x8000) {
				gp.iLocX += 3;
				RedrawImageOnBitmap(&gp);
			}
			if (GetKeyState('S') & 0x8000) {
				gp.iLocY -= 3;
				RedrawImageOnBitmap(&gp);
			}
			if (GetKeyState('D') & 0x8000) {
				gp.iLocX -= 3;
				RedrawImageOnBitmap(&gp);
			}
			RedrawImageOnBitmap(&gp);
		}

		MSG msg = { 0 };
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT) { running = false; }
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_CREATE: {

		BOOL enable = TRUE;
		DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

		break;
	}
	case WM_GETMINMAXINFO:
	{
		
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lparam;
		lpMMI->ptMinTrackSize.x = 340;
		lpMMI->ptMinTrackSize.y = 40 + 70;
		
		
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (!ToolbarMouseDown(&gp)) {
			return 0;
		}
	}
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN: {
		MouseDown(&gp);
		break;
	}
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP: {
		MouseUp(&gp);
		break;
	}
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE: {
		MouseMove(&gp);
		break;
	}
	case WM_KEYDOWN: {

		KeyDown(&gp, wparam, lparam);
		break;
	}
	case WM_SIZE: {
		Size(&gp);
		break;
	}
	case WM_MOUSEWHEEL: {
		MouseWheel(&gp, wparam, lparam);
		break;
	}
	case WM_CLOSE: {
		if (doIFSave(&gp)) {
			DestroyWindow(hwnd);
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	case WM_ACTIVATEAPP: {
		return 0;
	}
	case WM_PAINT: {

		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = gp.width;
		bmi.bmiHeader.biHeight = -(int64_t)gp.height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		SetDIBitsToDevice(gp.hdc, 0, 0, gp.width, gp.height, 0, 0, 0, gp.height, gp.scrdata, &bmi, DIB_RGB_COLORS);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	default: {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	}
	return 0;
}

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")