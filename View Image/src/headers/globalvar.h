#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <functional>

struct GlobalParams {
	// the global window
	HWND hwnd;
	HDC hdc;

	// memory
	void* imgdata;
	void* imgannotate;
	void* scrdata;
	void* toolbar_gaussian_data;
	uint32_t* tempCombineBuffer;
	// images
	unsigned char* toolbarData;
	unsigned char* toolbarData_shadow;
	unsigned char* mainToolbarCorner;
	unsigned char* fullscreenIconData;
	
	std::vector<uint32_t*> undoData;
	void* tempResizeBuffer;
	int undoStep = 0;

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
	int maxButtons = 11;
	const int iconSize = 30;
	int toolheight = 43;

	// current state

	bool shouldSaveShutdown = false;

	bool loading = false;

	bool halt = false;

	bool mouseDown = false;
	bool toolmouseDown = false;
	bool drawmousedown = false;

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

		int lastMoveX;
		int lastMoveY;

	bool fullscreen = false;
	WINDOWPLACEMENT wpPrev;

	// iterators
	std::vector<uint32_t> ith, itv;


	bool isMenuState = false;

	int menuY = toolheight+5;
	int menuX = 0;
	int menuSX = 0;
	int menuSY = 0;
	int mH = 25;

	bool smoothing = true;

	std::vector<std::pair<std::string, std::function<bool()>>> menuVector;

	// drawing/annotating

	bool drawmode = false;
	uint32_t a_drawColor = 0xCC0202;
	float drawSize = 20;
	int a_frost = 1;
	float a_opacity = 1.0f;

	bool isAnnotationCircleShown = false;

	bool pinkTestCenter = false;
};
