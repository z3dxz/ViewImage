#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <functional>


struct ToolbarButtonItem {
	int indexX = 0;
	std::string name;
	bool isSeperator;
};

struct MenuItem {
	std::string name; // has seperator in it
	std::function<bool()> func;
	int atlasX;
	int atlasY;
};

struct UndoDataStruct {
	uint32_t imageID;
	uint32_t imageIDoriginal;
	int width;
	int height;
};

#define REAL_BIG_VERSION_BOOLEAN "2.5Dv"

struct GlobalParams {
	std::vector<ToolbarButtonItem> toolbartable;

	// deltatime
	float ms_time = 0;

	// the global window
	HWND hwnd;
	HDC hdc;

	// memory
	void* imgdata;
	void* imgoriginaldata;
	void* scrdata;
	void* toolbar_gaussian_data;

	void* imagepreview;
	bool isImagePreview = false;

	// images
	unsigned char* toolbarData;
	unsigned char* im;
	unsigned char* fullscreenIconData;
	unsigned char* cropImageData;
	unsigned char* menu_icon_atlas;
	
	std::vector<UndoDataStruct> undoData;
	int undoStep = 0;
	int ProcessOfMakingUndoStep = 0;
	std::string undofolder;

	// strings
	std::string fpath;
	std::string renderfpath;
	std::string cd;

	// size integers
	int width = 1024;
	int height = 576;
	int imgwidth;
	int imgheight;

	int widthos; // for toolbar size
	int heightos;
	int channelos;

	int dumpchannel; // UNUSED
	int menu_atlas_SizeX;
	int menu_atlas_SizeY;

	// settings
	const int iconSize = 30; // the real width is 31 because 1 px divider
	int toolheight = 43;

	// current state

	bool shouldSaveShutdown = false;

	bool loading = false;
	bool deletingtemporaryfiles = false;

	bool halt = false;

	bool mouseDown = false;
	bool toolmouseDown = false;
	bool drawmousedown = false;
	int drawtype = 1; // 1 for draw: 0 for erase (override by control)

	int selectedbutton = -1;

	float iLocX = 0; // X position
	float iLocY = 0; // Y Position
	float mscaler = 1.0f; // Global zoom

	int CoordLeft = 0;
	int CoordTop = 0;
	int CoordRight = 0;
	int CoordBottom = 0;
	
	// mouse stuff
		bool lock = true;
		int lockimgoffx;
		int lockimgoffy;
		POINT LockmPos;
		bool isSize;
		int lastMouseX;
		int lastMouseY; // for drawing only


	bool fullscreen = false;
	WINDOWPLACEMENT wpPrev;

	// iterators
	std::vector<uint32_t> ith, itv;


	bool isMenuState = false;

	int menuY = toolheight+5;
	int menuX = 0;

	int actmenuY = 0;
	int actmenuX = 0;

	int menuSX = 0;
	int menuSY = 0;
	int mH = 25;

	// draw menu
	int drawMenuOffsetX = 0;
	int drawMenuOffsetY = 0;

	bool IsLastMouseDownWhenOverMenu = false;

	bool smoothing = true;

	std::string fontsfolder;

	std::vector<MenuItem> menuVector;

	// drawing/annotating

	
	bool drawmode = false;

	bool eyedroppermode = false;

	bool a_softmode = false;
	uint32_t a_drawColor = 0xFFCC0000f;
	float drawSize = 20;
	float a_opacity = 1.0f;
	float a_resolution = 30.0f;

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

	int mouseRightInitialCheckX;
	int mouseRightInitialCheckY;

	bool tint = false;

	// preloaded fonts

	FT_Face SegoeUI;
	FT_Face Verdana;
	FT_Face OCRAExt;
	FT_Face Tahoma;

	float etime = 0; // USED FOR MEASURING TIME

	// crop mode
	bool isInCropMode = false;
	float leftP = 0.0f;
	float rightP = 1.0f;
	float topP = 0.0f;
	float bottomP = 1.0f;
	bool CropHandleSelectTL = false;
	bool CropHandleSelectTR = false;
	bool CropHandleSelectBL = false;
	bool CropHandleSelectBR = false;

	bool isMovingTL = false;
	bool isMovingTR = false;
	bool isMovingBL = false;
	bool isMovingBR = false;

	// For WASD Protection ONLY
	float TransferWASDMoveMomentiumXIntArithmetic = 0;
	float TransferWASDMoveMomentiumYIntArithmetic = 0;
	bool SetLastMouseForWASDInputCaptureProtectionLock = false;


	bool isJoystick = false;
	JOYINFOEX joyInfoEx;
	UINT joystickID = JOYSTICKID1;
};


// mod list
// drawing (does not change original)
// resizing (does change original)
// rotating (does change original)
// effects (does not change original)
	// brightness contrast
	// gaussian
	// invert
// crop (does change original)