#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <functional>

struct UndoDataStruct {
	uint32_t* image;
	uint32_t* noannoimage;
	int width;
	int height;
};

#define REAL_BIG_VERSION_BOOLEAN "2.4Dv"

struct GlobalParams {

	// deltatime
	float ms_time = 0;

	bool fontinit = false;
	// the global window
	HWND hwnd;
	HDC hdc;

	// memory
	void* imgdata;
	void* imgoriginaldata;
	void* scrdata;
	void* toolbar_gaussian_data;
	// images
	unsigned char* toolbarData;
	unsigned char* toolbarData_shadow;
	unsigned char* mainToolbarCorner;
	unsigned char* fullscreenIconData;
	
	std::vector<UndoDataStruct> undoData;
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
	int maxButtons = 12;
	const int iconSize = 30; // the real width is 31 because 1 px divider
	int toolheight = 43;

	// current state

	bool shouldSaveShutdown = false;

	bool loading = false;

	bool halt = false;

	bool mouseDown = false;
	bool toolmouseDown = false;
	bool drawmousedown = false;
	int drawtype = 1; // 1 for draw: 0 for erase (override by control)

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

	// iterators
	std::vector<uint32_t> ith, itv;


	bool isMenuState = false;

	int menuY = toolheight+5;
	int menuX = 0;
	int menuSX = 0;
	int menuSY = 0;
	int mH = 25;

	// draw menu
	int drawMenuOffsetX = 393;
	int drawMenuOffsetY = 0;

	bool smoothing = true;

	std::string fontsfolder;

	std::vector<std::pair<std::string, std::function<bool()>>> menuVector;

	// drawing/annotating

	
	bool drawmode = false;

	bool eyedroppermode = false;

	bool a_softmode = false;
	uint32_t a_drawColor = 0xFFCC0000f;
	float drawSize = 20;
	float a_opacity = 1.0f;
	float a_resolution = 30.0f;

	bool isAnnotationCircleShown = false;

	bool pinkTestCenter = false;

	// slider's

	int slider1begin = 113;
	int slider1end = 214;

	int slider2begin = 325;
	int slider2end = 405;

	bool slider1mousedown = false;
	bool slider2mousedown = false;
	bool slider3mousedown = false;
	bool slider4mousedown = false;

	float testfloat = 0.0f;

	bool debugmode = false;

	bool sleepmode = false;

};


// mod list
// drawing
// resizing
// rotating