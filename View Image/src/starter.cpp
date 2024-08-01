
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include "../resource.h"
#include <cstdint>
#include "headers/ops.hpp"
//#include <dwmapi.h>
#include <uxtheme.h>
#include "headers/globalvar.hpp"
#include "headers/imgload.hpp"
#include "headers/events.hpp"

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")
// draw vars

GlobalParams gp;


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


/*
 ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
 ▐░▌           ▐░▌  ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌       ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
  ▐░▌         ▐░▌       ▐░▌     ▐░▌          ▐░▌       ▐░▌     ▐░▌     ▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌▐░▌          ▐░▌
   ▐░▌       ▐░▌        ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌   ▄   ▐░▌     ▐░▌     ▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░▌ ▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄
	▐░▌     ▐░▌         ▐░▌     ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░▌▐░░░░░░░░▌▐░░░░░░░░░░░▌
	 ▐░▌   ▐░▌          ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌ ▐░▌░▌ ▐░▌     ▐░▌     ▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░▌ ▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀
	  ▐░▌ ▐░▌           ▐░▌     ▐░▌          ▐░▌▐░▌ ▐░▌▐░▌     ▐░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌▐░▌
	   ▐░▐░▌        ▄▄▄▄█░█▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌░▌   ▐░▐░▌ ▄▄▄▄█░█▄▄▄▄ ▐░▌       ▐░▌▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄
		▐░▌        ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
		 ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀       ▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀

 Welcome! The program starts here. If you have any questions about the code, please talk to me at github.com

 Note: Although the code is C++, the code is mostly in C style. Object oriented programming is not apparent and "classes" are just
 structs being passed through arguments


Thank you Sean T. Barrett for making stb_image(_resize/_write) and this entire image viewer possible


*/


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
	//wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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

	gp.hwnd = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, px, py, w_width, w_height, NULL, NULL, NULL, NULL);



	// RIGHT HERE: WIDTH = 0

	gp.hdc = GetDC(gp.hwnd);

	if (!Initialization(&gp, argc, argv)) {
		return 0;
	}

	// WASD Replaced with TIMER! Look under WNDPROC Below
	SetTimer(gp.hwnd, 1, 12, NULL);

	MSG msg{};
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}






LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {


	switch (msg) {
	case WM_CREATE: {


		//BOOL enable = TRUE;
		//DwmSetWindowAttribute(hwnd, 20, &enable, sizeof(enable));

		if (!DwmDarken(hwnd)) {
			// windows XP
		}

		break;
	}
	case WM_TIMER: {

		PerformWASDMagic(&gp);

		break;
	}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lparam;
		lpMMI->ptMinTrackSize.x = 505;
		lpMMI->ptMinTrackSize.y = 184;


		break;
	}
	case WM_LBUTTONDOWN:
	{
		if (!ToolbarMouseDown(&gp)) {
			return 0;
		}
	}
	case WM_MBUTTONDOWN: {
		MouseDown(&gp);
		break;
	}
	case WM_LBUTTONUP:
	case WM_MBUTTONUP: {
		MouseUp(&gp);
		break;
	}
	case WM_RBUTTONUP: {

		RightUp(&gp);

		break;
	}
	case WM_RBUTTONDOWN: {
		RightDown(&gp);
		break;
	}
	case WM_MOUSEMOVE: {
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
	case WM_KEYUP: {
		RedrawSurface(&gp);
		break;
	}
	case WM_MOUSEWHEEL: {
		MouseWheel(&gp, wparam, lparam);
		break;
	}
	case WM_CLOSE: {
		if (doIFSave(&gp)) {
			gp.loading = true;
			RedrawSurface(&gp);
			DeleteTempFiles(&gp);
			DestroyWindow(hwnd);
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	//case WM_ACTIVATEAPP: {
	//	return 0;
	//}
	case WM_PAINT: {
		UpdateBuffer(&gp);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	case WM_SETFOCUS: {
		gp.sleepmode = false;
		if (gp.scrdata && gp.width > 1) {
			RedrawSurface(&gp);
		}
		break;
	}
	case WM_KILLFOCUS: {

		gp.sleepmode = true;
		for (int i = 0; i < 50; i++) {
			ShowCursor(500);
		}
		break;
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