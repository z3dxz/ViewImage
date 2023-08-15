#pragma once
#include <string>
#include <Windows.h>

struct GlobalParams {
	// the global window
	HWND hwnd;
	HDC hdc;

	// memory
	void* imgdata;
	void* scrdata;
	unsigned char* toolbarData;

	// strings
	std::string fpath;
	std::string renderfpath;
	std::string cd;

	// size integers
	int width = 1024;
	int height = 576;
	int imgwidth;
	int imgheight;

	int widthos;
	int heightos;
	int channelos;

	// settings
	int maxButtons = 9;
	const int iconSize = 30;
	int toolheight = 43;

	// current state

	bool shouldSaveShutdown = false;

	bool loading = false;

	bool halt = false;

	bool mouseDown = false;

	int selectedbutton = -1;

	float iLocX = 0; // X position
	float iLocY = 0; // Y Position
	float mscaler = 1.0f; // Global zoom

	int CoordLeft;
	int CoordTop;
	int CoordRight;
	int CoordBottom;
	
	// mouse stuff
		bool lock = true;
		int lockimgoffx;
		int lockimgoffy;
		POINT LockmPos;
		bool isSize;
		int lastMouseX;
		int lastMouseY;

	bool fullscreen = false;
	WINDOWPLACEMENT wpPrev;

};
