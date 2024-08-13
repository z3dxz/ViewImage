#include "headers/events.hpp" // e
#include "../resource.h"
#include <Shlwapi.h>
#include <vector>
#include <fstream>
#include <thread>
#include <limits>
#include <future>
#include <string>
#include <iostream>
#include <sstream>



void ToggleFullscreen(GlobalParams* m);


#include <future>     // for std::async
#include <mutex>      // for std::mutex, std::unique_lock
#include <condition_variable> // for std::condition_variable
#include <random>

std::mutex mtx;
std::condition_variable cv;
bool resizeCompleted = false;

void ResizeBuffers(GlobalParams* m) {
	RECT ws = { 0 };
	GetClientRect(m->hwnd, &ws);
	int newWidth = ws.right - ws.left;
	int newHeight = ws.bottom - ws.top;

	if (newWidth < 1) { newWidth = 1; }
	if (newHeight < 1) { newHeight = 1; }

	// Allocate new buffers
	void* newScrdata = realloc(m->scrdata, newWidth * newHeight * 4);
	void* newToolbarData = realloc(m->toolbar_gaussian_data, newWidth * m->toolheight * 4);
	std::vector<uint32_t> newIth(newWidth);
	std::vector<uint32_t> newItv(newHeight);

	if (newScrdata && newToolbarData) {
		// Update with new data
		m->width = newWidth;
		m->height = newHeight;
		m->scrdata = newScrdata;
		m->toolbar_gaussian_data = newToolbarData;
		m->ith = std::move(newIth);
		m->itv = std::move(newItv);
		for (uint32_t i = 0; i < m->width; i++)
			m->ith[i] = i;
		for (uint32_t i = 0; i < m->height; i++)
			m->itv[i] = i;
	}
	else {
		// Error handling: Failed to allocate memory
		if (newScrdata) FreeData(newScrdata);
		if (newToolbarData) FreeData(newToolbarData);
	}

	// Notify main thread
	{
		std::lock_guard<std::mutex> lock(mtx);
		resizeCompleted = true;
	}
	cv.notify_one();
}

void Size(GlobalParams* m) {
	if (m->scrdata) {
		// Reset completion flag
		{
			std::lock_guard<std::mutex> lock(mtx);
			resizeCompleted = false;
		}

		// Asynchronously resize buffers
		std::future<void> asyncResize = std::async(std::launch::async, ResizeBuffers, m);

		// Wait for the async operation to complete or timeout
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (!cv.wait_for(lock, std::chrono::milliseconds(1000), [] { return resizeCompleted; })) {
				// Handle timeout: async operation took too long
				// Optionally, cancel asyncResize or retry
			}
		}
		// Continue with other operations after resizing (if needed)
		autozoom(m);
		RedrawSurface(m);
	}
}

HANDLE hMutex;
// initiz

void parseCommandLine(const std::string& cmdLine, std::string& firstArg, std::string& secondArg, std::string& thirdArg) {
	std::istringstream stream(cmdLine);
	std::string token;
	std::vector<std::string> args;
	bool inQuotes = false;

	while (stream >> std::ws) {
		if (stream.peek() == '"') {
			stream.ignore();
			std::getline(stream, token, '"');
		}
		else {
			stream >> token;
		}
		args.push_back(token);
	}

	// Skip the application name
	if (args.size() > 1) firstArg = args[1];
	if (args.size() > 2) secondArg = args[2];
	if (args.size() > 3) thirdArg = args[3];

	if (args.size() <= 1) firstArg = "";
	if (args.size() <= 2) secondArg = "";
	if (args.size() <= 3) thirdArg = "";
}

bool Initialization(GlobalParams* m, int argc, LPWSTR* argv) {
	
	
	m->toolbartable = {
	   {0,   "Open Image (F)" , false},
	   {31,  "Save as PNG (CTRL+S)", true},
	   {62,  "Zoom In", false},
	   {93,  "Zoom Out", false},
	   {124, "Zoom Auto", false},
	   {155, "Zoom 1:1 (100%)", true},
	   {186, "Rotate", false},
	   {217, "Annotate (G)", false},
	   {248, "Image Operations", true},
	   {279, "DELETE image", false},
	   {310, "Print", false},
	   {341, "Copy Image", true},
	   {372, "Information", false}
	};

	

	char buffer[MAX_PATH];
	DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
	PathRemoveFileSpec(buffer);

	m->cd = std::string(buffer);

	//InitFont(hwnd, "segoeui.TTF", 14);

	// load fonts here


	m->Verdana = LoadFont(m, "Verdana.ttf");
	m->SegoeUI = LoadFont(m, "SegoeUI.ttf");
	m->OCRAExt = LoadFont(m, "OCRAEXT.ttf");
	m->Tahoma = LoadFont(m, "Tahoma.ttf");

	size_t scrsize = m->width * m->height * 4;
	m->scrdata = malloc(scrsize);
	for (uint32_t i = 0; i < m->width * m->height; i++) {
		*((uint32_t*)m->scrdata+i) = 0x008080; // TEAL color
	}

	m->toolbar_gaussian_data = malloc(m->width * m->toolheight * 4);

	m->toolbarData = LoadImageFromResource(TOOLBAR_RES, m->widthos, m->heightos, m->channelos);




	int null1, null2, null3;

	// icons
	m->menu_icon_atlas = LoadImageFromResource(ICON_MAP, m->menu_atlas_SizeX, m->menu_atlas_SizeY, m->dumpchannel);
	m->fullscreenIconData = LoadImageFromResource(FS_ICON, null1, null2, null3);
	m->cropImageData = LoadImageFromResource(CROPICON, null1, null2, null3);


	// TEMPORARY FILES STUFF

	DWORD pathLength = MAX_PATH;
	TCHAR tempPath[MAX_PATH];
	GetTempPath(pathLength, tempPath);
	if (!tempPath) {
		MessageBox(m->hwnd, "Unable to locate windows temporary files path", "Error", MB_OK | MB_ICONERROR);
		exit(0);
		return false;
	}

	std::string firstfolder = std::string(tempPath) + "View Image";
	CreateDirectory(firstfolder.c_str(), NULL);
	
	hMutex = CreateMutex(NULL, TRUE, "ViewImageMainProcessUndoCatalogPackagesTemporaryFilesMutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Great!
	} 
	else {
		bool isempty = PathIsDirectoryEmpty(firstfolder.c_str());
		if (!isempty) {
			//int r = MessageBox(m->hwnd, "It appears that View Image has not been shutdown correctly last session\nThe temporary files are still present. It is highly recommended that you delete these for improved disk space and performance. \nWould you like to remove excess temporary files?", "Warning", MB_ICONWARNING | MB_YESNO);
			m->deletingtemporaryfiles = true;
			RedrawSurface(m);

			// for notice
			m->deletingtemporaryfiles = false;
			RedrawSurface(m);
			//if (r == IDYES) {
				// delete
			if (!DeleteDirectory(firstfolder.c_str())) {
				DWORD error = GetLastError();
				std::string s = "Failed to remove temporary files: ERR CODE: " + std::to_string(error);
				MessageBox(m->hwnd, "Failed to remove the temporary files for this application. You can manually remove them at AppData\\Local\\Temp, with all containing view image directories", s.c_str(), MB_OK | MB_ICONERROR);
			}
			CreateDirectory(firstfolder.c_str(), NULL);
			//}
		}
	}

	int PID = GetCurrentProcessId();

	m->undofolder = std::string(tempPath) + "View Image" + "\\VIEW_IMAGE_TEMPORARY_DATA_" + std::to_string(PID) + "\\";
	if (!CreateDirectory(m->undofolder.c_str(), NULL)) {
		MessageBox(m->hwnd, "Unable to create process-specific temporary files folder", "Error", MB_OK | MB_ICONERROR);
		exit(0);
		return false;
	}
	
	// END OF TEMP FILES


	// load pathway image
	
		// turn arguments into path
		/*
		
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);

		
		char* path = (char*)malloc(size_needed);
		
		WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, path, size_needed, NULL, NULL);
		
		if (argc >= 2) {
			if (!OpenImageFromPath(m, path, false)) {
				MessageBox(m->hwnd, "Unable to open image", "Error", MB_OK | MB_ICONERROR);
				exit(0);
				return false;
			}
		}
		else {
			RedrawSurface(m);
		}
		*/

	std::string cmdLine = std::string(GetCommandLineA());

	std::string firstArg;
	std::string secondArg;
	std::string thirdArg;

	parseCommandLine(cmdLine, firstArg, secondArg, thirdArg);


	if (secondArg != "") {
		if (!OpenImageFromPath(m, secondArg, false)) {
			MessageBox(m->hwnd, "Unable to open image", "Error", MB_OK | MB_ICONERROR);
			exit(0);
			return false;
		}
	}
		
	Size(m);

	if (thirdArg != "") {
		if (thirdArg == "--full") {
			ToggleFullscreen(m);
		}
	}

	/*
	// init joystick
	m->joyInfoEx.dwSize = sizeof(JOYINFOEX);
	m->joyInfoEx.dwFlags = JOY_RETURNALL;

	if (joyGetPosEx(m->joystickID, &m->joyInfoEx) != JOYERR_NOERROR) {
		MessageBox(NULL, "Joystick not found!", "Error", MB_OK | MB_ICONERROR);
	}
	else {
		m->isJoystick = true;
	}
	
	*/
	return true;
}

uint32_t PickColorFromDialog(GlobalParams* m, uint32_t def, bool* success) {
	CHOOSECOLOR cc;
	COLORREF acrCustClr[16];
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = m->hwnd; // If you have a window handle, you can set it here
	cc.lpCustColors = (LPDWORD)acrCustClr;
	cc.rgbResult = def; // Default color
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&cc) == TRUE) {
		return (uint32_t)cc.rgbResult; // Convert COLORREF to uint32_t
	}
	else {
		// Handle error or cancellation
		*success = false;
		return 0; // Assuming 0 as an invalid color value
	}
}

static DWORD previousTime;
void PerformWASDMagic(GlobalParams* m) {

	DWORD currentTime = timeGetTime();
	DWORD deltaTime = currentTime - previousTime;
	previousTime = timeGetTime();

	UpdateBuffer(m);
	HWND temp = GetActiveWindow();
	if (temp == m->hwnd) {

		bool isW = GetKeyState('W') & 0x8000;
		bool isA = GetKeyState('A') & 0x8000;
		bool isS = GetKeyState('S') & 0x8000;
		bool isD = GetKeyState('D') & 0x8000;


		if (isW) {
			 m->TransferWASDMoveMomentiumYIntArithmetic += (deltaTime / 16.0f);
		}
		if (isA) {
			m->TransferWASDMoveMomentiumXIntArithmetic += (deltaTime / 16.0f);
		}
		if (isS) {
			m->TransferWASDMoveMomentiumYIntArithmetic -= (deltaTime / 16.0f);
		}
		if (isD) {
			m->TransferWASDMoveMomentiumXIntArithmetic -= (deltaTime / 16.0f);
		}


		if (abs(m->TransferWASDMoveMomentiumXIntArithmetic) > 0.01f || abs(m->TransferWASDMoveMomentiumYIntArithmetic) > 0.01f) {

			if (m->TransferWASDMoveMomentiumXIntArithmetic > 1.0f) {
				m->TransferWASDMoveMomentiumXIntArithmetic = 1.0f;
			}
			if (m->TransferWASDMoveMomentiumXIntArithmetic < -1.0f) {
				m->TransferWASDMoveMomentiumXIntArithmetic = -1.0f;
			}

			if (m->TransferWASDMoveMomentiumYIntArithmetic > 1.0f) {
				m->TransferWASDMoveMomentiumYIntArithmetic = 1.0f;
			}
			if (m->TransferWASDMoveMomentiumYIntArithmetic < -1.0f) {
				m->TransferWASDMoveMomentiumYIntArithmetic = -1.0f;
			}

			float speed = ((m->mscaler - 1.0f) / 4.0f) + 1.0f;
			speed *= 0.75f;

			MouseMove(m, false);
			int advanceX = (deltaTime * m->TransferWASDMoveMomentiumXIntArithmetic) * speed;
			int advanceY = (deltaTime * m->TransferWASDMoveMomentiumYIntArithmetic) * speed;
			m->iLocX += advanceX;
			m->iLocY += advanceY;
			if (!m->SetLastMouseForWASDInputCaptureProtectionLock) {
				m->lastMouseX += advanceX;
				m->lastMouseY += advanceY;
			}
			else {
			}

			m->TransferWASDMoveMomentiumXIntArithmetic *= 0.9f;
			m->TransferWASDMoveMomentiumYIntArithmetic *= 0.9f;
			RedrawSurface(m);
		}


		if (isW || isA || isS || isD) {
		}
	}
}

void OpenImageEffectsMenu(GlobalParams* m) {
	m->menuVector = {

		{"Automatic Adjust",
			[m]() -> bool {
				AutoAdjustLevels(m, (uint32_t*)m->imgdata);
				return true;
			},78,13
		},

		{"Brightness/Contrast",
			[m]() -> bool {
				m->isMenuState = false;
				RedrawSurface(m);
				ShowBrightnessContrastDialog(m);
				return true;
			},65,0
		},

		{"Invert Colors",
			[m]() -> bool {
				m->isMenuState = false;
				// modify
				m->shouldSaveShutdown = true;
				createUndoStep(m, false);
				for (int y = 0; y < m->imgheight; y++) {
					for (int x = 0; x < m->imgwidth; x++) {
						uint32_t* loc = GetMemoryLocation(m->imgdata, x, y, m->imgwidth, m->imgheight);
						// seperate RGB
						uint32_t color = *loc;

						uint8_t a = (color >> 24) & 0xFF;
						uint8_t r = (color >> 16) & 0xFF;
						uint8_t g = (color >> 8) & 0xFF;
						uint8_t b = color & 0xFF;

						r = 255 - r; g = 255 - g; b = 255 - b;
						*loc = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}
				RedrawSurface(m);
				return true;
			},65,13
		},


		{"Gaussian Blur",
			[m]() -> bool {
				m->isMenuState = false;
				RedrawSurface(m);
				ShowGaussianDialog(m);
				return true;
			},91,0
		},

		{"Draw Text{s}",
			[m]() -> bool {

				m->isMenuState = false;
				RedrawSurface(m);
				ShowDrawTextDialog(m);
				return true;
			},0,13
		},

		{"Crop Image",
			[m]() -> bool {
				autozoom(m);
				m->mscaler = m->mscaler * 0.8f;
				TurnOffDraw(m);
				m->drawmode = false;
				m->isInCropMode = true;
				RedrawSurface(m);

				return true;
			},52,13
		},

		{"Erase annotations",
			[m]() -> bool {
			
				int result = MessageBox(m->hwnd, "This will remove all annotations from the image, but keep its proportions", "Are You Sure?", MB_YESNO);
				createUndoStep(m,false);
				if (result == IDYES) {
					memcpy(m->imgdata, m->imgoriginaldata, m->imgwidth * m->imgheight * 4);
				}


				return true;
			},78,0
		},

	};

	m->menuX = GetLocationFromButton(m, 8); // 8 = effects
	m->menuY = m->toolheight;
	m->isMenuState = true;
	RedrawSurface(m);
}
void ShowMyInformation(GlobalParams* m) {
	char str[256];

	std::string txt = "No Image Loaded";
	if (!m->fpath.empty()) {
		txt = m->fpath;
	}

	sprintf(str, "\nCurrently Loaded: \n%s\n\nAttributes (in memory):\nWidth: %d\nHeight: %d\n\nAbout:\nVisit cosine64.com for more quality software\nVersion: %s", txt.c_str(), m->imgwidth, m->imgheight, REAL_BIG_VERSION_BOOLEAN);

	MessageBox(m->hwnd, str, "Information", MB_OK);
}

int PerformCasedBasedOperation(GlobalParams* m, uint32_t id, bool menustate) {
	if (m->imgwidth > 0 || id == 0) {}
	else { return 1; }
	switch (id) {
	case 0:
		// open
		PrepareOpenImage(m);
		return 0;
	case 1:
		// save

		PrepareSaveImage(m);

		return 0;
	case 2:
		// zoom in
		NewZoom(m, 1.25f, 2, true); // ALSO: Use this for the + and - hotkeys for ZOOM
		return 0;
	case 3:
		// zoom out
		NewZoom(m, 0.8f, 2, true); // ALSO HERE TOO 
		return 0;
	case 4:
		// zoom fit
		autozoom(m);
		RedrawSurface(m);
		return 0;
	case 5:
		// zoom original
		m->mscaler = 1.0f;
		RedrawSurface(m);

		return 0;
	case 6:
		// rotate

		// rotate = later
		rotateImage90Degrees(m);
		//OpenImageFromPath(m, cpath.c_str());

		//MessageBox(hwnd, "I haven't added this feature yet", "Can't rotate image", MB_OK | MB_ICONERROR);
		return 0;
	case 7: {
		// draw
		m->drawmode = !m->drawmode;
		RedrawSurface(m);
		return 0;
	}
	case 8: {
		// effects
		if (menustate) {
			m->isMenuState = false;
			break;
		}
		OpenImageEffectsMenu(m);
		return 0;
	}
	case 9: {
		// DELETE
		if (m->fpath != "Untitled") {

			// delete
			int result = MessageBox(m->hwnd, "This will delete the image permanently!!!", "Are You Sure?", MB_YESNO);
			if (result == IDYES) {
				bool i = DeleteFile(m->fpath.c_str());
				if (!i) {
					int err = GetLastError();
					MessageBox(m->hwnd, std::string("Can not delete file - Error: " + std::to_string(err)).c_str(), "Error", MB_OK | MB_ICONERROR);
					return 0;
				}
				CHAR szPath[MAX_PATH];
				GetModuleFileName(NULL, szPath, MAX_PATH);

				// Start a new instance of the application
				ShellExecute(NULL, "open", szPath, NULL, NULL, SW_SHOWNORMAL);

				// Terminate the current instance of the application
				ExitProcess(0);
			}
		}
		else {
			MessageBox(m->hwnd, "The image that is loaded is temporary and can not be deleted", "Untitled", MB_OK);
		}


		return 0;
	}
	case 10: {
		// print
		Print(m);

		return 0;
	}
	case 11: {

		// copy
		if (!CopyImageToClipboard(m, m->imgdata, m->imgwidth, m->imgheight)) {
			MessageBox(m->hwnd, "Error copying the image to the clipboard", "Bug Detected!", MB_OK | MB_ICONERROR);
		}

		return 0;
	}
	case 12: {


		// information
		// image info
		
		ShowMyInformation(m);
		return 0;
	}
	}

	return 1;
}

void CloseMenuWhenInactive(GlobalParams* m, POINT& k) {
	if (IfInMenu(k, m)) {

	}
	else {

		m->isMenuState = false;
		RedrawSurface(m);
	}
}



bool test1 = false;
bool ToolbarMouseDown(GlobalParams* m) {
	m->IsLastMouseDownWhenOverMenu = false;

	test1 = true;

	if (m->isInCropMode) {
		if (m->imgwidth < 1) {
			PrepareOpenImage(m);
		}

		m->isMovingTL = m->CropHandleSelectTL;
		m->isMovingTR = m->CropHandleSelectTR;
		m->isMovingBL = m->CropHandleSelectBL;
		m->isMovingBR = m->CropHandleSelectBR;

		return 0;
	}

	if (m->eyedroppermode) {
		return 0;
	}
	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);
	bool prevmenustate = m->isMenuState;
	if (prevmenustate) {
		CloseMenuWhenInactive(m, mPP);
		//return 1;
	}

	if (IfInMenu(mPP, m)&&m->isMenuState) {
		m->IsLastMouseDownWhenOverMenu = true;
		return 1;
	}


	if (m->drawmode)
	{
		// this is for the drawing toolbar ONLY

		int colorBeginX = m->drawMenuOffsetX + 51;
		int colorEndX = m->drawMenuOffsetX + 69;
		int colorBeginY = m->drawMenuOffsetY + 14;
		int colorEndY = m->drawMenuOffsetY + 32;

		int slider1begin = m->drawMenuOffsetX + m->slider1begin;
		int slider1end = m->drawMenuOffsetX + m->slider1end;

		int slider2begin = m->drawMenuOffsetX + m->slider2begin;
		int slider2end = m->drawMenuOffsetX + m->slider2end;

		int sliderYb = m->drawMenuOffsetY;
		int sliderYe = m->drawMenuOffsetY + 40;

		int softBeginX = m->drawMenuOffsetX + 461;
		int softEndX = m->drawMenuOffsetX + 479;
		int softBeginY = m->drawMenuOffsetY + 14;
		int softEndY = m->drawMenuOffsetY + 32;

		if ((mPP.x > colorBeginX && mPP.x < colorEndX) && (mPP.y > colorBeginY && mPP.y < colorEndY)) { //color icon coordinates
			// open color picker
			bool success = true;// alpha to 0 weird windows bug
			uint32_t c = change_alpha(PickColorFromDialog(m, InvertCC(change_alpha(m->a_drawColor,0),true), &success), 255);
			if (success) {
				m->a_drawColor = InvertCC(c, true);
			}
			RedrawSurface(m);
			return 0;
		}
		if ((mPP.x > softBeginX && mPP.x < softEndX) && (mPP.y > softBeginY && mPP.y < softEndY)) { //color icon coordinates
			// open color picker
			m->a_softmode = !m->a_softmode;
			RedrawSurface(m);
			return 0;
		}


		if (CheckIfMouseInSlider1(mPP, m, slider1begin, slider1end, sliderYb, sliderYe)) { // these are also found in the scroll USE CONTROL F
			m->slider1mousedown = true;
		}

		if (CheckIfMouseInSlider2(mPP, m, slider2begin, slider2end, sliderYb, sliderYe)) {
			m->slider2mousedown = true;
		}

	}

	m->toolmouseDown = true;
	if (m->drawmode) {
		if (IsInImage(mPP, m)) {
			m->drawtype = 1;
			TurnOnDraw(m);
			createUndoStep(m, true);
		}
	}

	// add toolbar clicky here
	{
		// fullscreen icon
		if ((mPP.x > m->width - 36 && mPP.x < m->width - 13) && (mPP.y > 12 && mPP.y < 33)) { //fullscreen icon location check coordinates (ALWAYS KEEP)
			if (m->width > 535) { // to check to make sure window isn't too small
				ToggleFullscreen(m); // TODO: please make a seperate icon for the exiting fullscreen
			}

			return 0;
		}
	}


	uint32_t id = getXbuttonID(m, mPP);

	return PerformCasedBasedOperation(m, id, prevmenustate);
}

void MouseDown(GlobalParams* m) {
	test1 = true;
	if (m->isInCropMode) {
		return;
	}

	if (m->eyedroppermode) {
		return;
	}
	m->lastMouseX = -1;
	m->lastMouseY = -1;

	m->lockimgoffx = m->iLocX;
	m->lockimgoffy = m->iLocY;
	GetCursorPos(&m->LockmPos);
	POINT k;
	GetCursorPos(&k);
	ScreenToClient(m->hwnd, &k);

	CloseMenuWhenInactive(m, k);
	
	if (k.y > m->toolheight) {
		if(!m->isMenuState)
		m->mouseDown = true;
	}
	MouseMove(m);
}




POINT* sampleLine_old(double x1, double y1, double x2, double y2, int numSamples) {


	POINT* samples = (POINT*)malloc(sizeof(POINT) * numSamples);

	for (int i = 0; i < numSamples; i++) {
		double t = (double)i / (numSamples - 1);
		samples[i].x = x1 + t * (x2 - x1);
		samples[i].y = y1 + t * (y2 - y1);
	}

	// remember to deallocate
	return samples;
}

void TurnOnDraw(GlobalParams* m) {
	if (!m->drawmousedown) {
		m->SetLastMouseForWASDInputCaptureProtectionLock = true;
		m->drawmousedown = true;
	}
}

void TurnOffDraw(GlobalParams* m) {
	if (m->drawmousedown) {
		m->drawmousedown = false;
	}
}

POINT* sampleLine(GlobalParams* m, double x1, double y1, double x2, double y2, int* outSamples) {
	float sizex = abs(x2 - x1);
	float sizey = abs(y2 - y1);
	float dist = sqrt(pow(sizex, 2) + pow(sizey, 2));
	int numSamples = dist / (m->drawSize/ m->a_resolution);

	int qualsamples = numSamples;
	if (numSamples > 2) { qualsamples--; };

	POINT* samples = (POINT*)malloc(sizeof(POINT) * (qualsamples + 1)); // Include starting point

	float dx = (x2 - x1) / numSamples;
	float dy = (y2 - y1) / numSamples;

	samples[0].x = x1;
	samples[0].y = y1;

	for (int i = 1; i <= qualsamples; i++) {
		samples[i].x = x1 + i * dx;
		samples[i].y = y1 + i * dy;
	}

	*outSamples = qualsamples + 1; // Include starting point
	return samples;
}



clock_t start, end;
double delta_time;


void placeDraw(GlobalParams* m, POINT* pos) {

	// draw goes here abcdefghijklmnopqrstuvwxyz
	start = clock();

	// automatic speed based resolution changer
	
	float target_delta_time = 0.016f;
	float delta_time_difference = target_delta_time - m->ms_time;
	
	float adjustment_factor = std::pow(2, delta_time_difference * 10);

	m->a_resolution *= adjustment_factor;
	m->a_resolution = max(1.0f, min(25.0f, m->a_resolution));
	
	if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
		//TurnOffDraw(m);
		m->mouseDown = false;
		m->toolmouseDown = false;
		return;
	}

    ScreenToClient(m->hwnd, pos);

    int k = (int)((float)((m->lastMouseX) - m->CoordLeft) * (1.0f / m->mscaler));
    int v = (int)((float)((m->lastMouseY) - m->CoordTop) * (1.0f / m->mscaler));

    int k1 = (int)((float)(pos->x - m->CoordLeft) * (1.0f / m->mscaler));
    int v1 = (int)((float)(pos->y - m->CoordTop) * (1.0f / m->mscaler));

	float sizex = abs(k - k1);
	float sizey = abs(v - v1);
	float dist = (sqrt(pow(sizex, 2) + pow(sizey, 2)));

	if (dist > (m->drawSize/ m->a_resolution)) {
		if (m->lastMouseX == -1) {
			k = k1; v = v1;
		}

		int samples0;
		POINT* k2 = sampleLine(m, k1, v1, k, v, &samples0);

		int diameter = m->drawSize;
		float radius = (float)diameter / 2.0f;
		
		float realOpacity = pow(m->a_opacity, 4);

		for (int i = 0; i < samples0; i++) {

			//auto start = std::chrono::high_resolution_clock::now();

			for (int y = 0; y < diameter; y++) {
				for (int x = 0; x < diameter; x++) {
					float virtual_x = (float)x + 0.5f;
					float virtual_y = (float)y + 0.5f;
					double distance = sqrt(pow(virtual_x - radius, 2) + pow(virtual_y - radius, 2)); // Circle formula

					if (distance <= radius) { // Inside the circle's bounding box
						uint32_t xloc = (virtual_x - radius) + (k2[i].x);
						uint32_t yloc = (virtual_y - radius) + (k2[i].y);

						if (xloc < m->imgwidth && yloc < m->imgheight) {
							uint32_t* memoryPath = GetMemoryLocation(m->imgdata, xloc, yloc, m->imgwidth, m->imgheight);
							uint32_t* memoryPathOriginal = GetMemoryLocation(m->imgoriginaldata, xloc, yloc, m->imgwidth, m->imgheight);
							uint32_t actualDrawColor = m->a_drawColor;

							if (GetKeyState(VK_CONTROL) & 0x8000 || (m->drawtype == 0)) {
								actualDrawColor = *memoryPathOriginal;
							}

							if (GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_SHIFT) & 0x8000) {
								actualDrawColor = 0x00000000;
							}
							float transparency = 0.0f;
							if (m->a_softmode) {
								transparency = pow(distance / radius, 2);
							}
							//alpha = (0.1f * alpha)+0.9f;
							if (m->drawSize > 1.01f) {
								float smoothening = 1.0f / (m->drawSize) * 5;
								//alpha = 5.0f*(alpha - 0.8f);
								if (!m->a_softmode) {
									//transparency = (smoothening + transparency - 1) / smoothening;
									transparency = distance-radius+1;
									if (transparency < 0.001f) {
										transparency = 0.0f;
									}
									if (transparency > 9.99f) {
										transparency = 1.0f;
									}
								}
								
								float radiusalpha = (1.0f - transparency) * realOpacity;

								if (m->a_opacity > 0.99f && radiusalpha > 0.995f) {
									*memoryPath = actualDrawColor;
								}
								else {
									if (m->a_resolution > 20) {
										m->etime = 500;
										*memoryPath = lerp_gc(*memoryPath, actualDrawColor, radiusalpha);
									}
									else {
										m->etime = 200;
										*memoryPath = lerp(*memoryPath, actualDrawColor, radiusalpha);
									}
								}
							}
							else {
								if (m->a_opacity > 0.99f) {
									*memoryPath = actualDrawColor;
								}
								else {
									*memoryPath = lerp_gc(*memoryPath, actualDrawColor, realOpacity);
								}
							}
							
							//CircleGenerator(m->drawSize, xloc, yloc,1, 4, (uint32_t*)m->imgdata, m->a_drawColor, m->a_opacity, m->imgwidth, m->imgheight);
							
						}
					}
				}
			}

			//auto end = std::chrono::high_resolution_clock::now();
			//std::chrono::duration<float, std::milli> duration = end - start;
			//m->etime = duration.count();

		}
		FreeData(k2);

		//Beep(500, 50);
		m->lastMouseX = pos->x;
		m->lastMouseY = pos->y;
		m->SetLastMouseForWASDInputCaptureProtectionLock = false;
		m->shouldSaveShutdown = true;
	}



	end = clock();
	delta_time = ((double)(end - start)) / CLOCKS_PER_SEC;


	m->ms_time = delta_time;


}


bool firsttime = true;

void GuidedRedrawSurface(GlobalParams* m) {
	ResetCoordinates(m);

	if (m->CoordTop > (m->toolheight-2)) {
		if (firsttime) {
			RedrawSurface(m);
			firsttime = false;
		}
		else {
			HRGN rgn = CreateRectRgn(0, m->toolheight, m->width, m->height);
			SelectClipRgn(m->hdc, rgn);
			RedrawSurface(m, true, true);
		}
	}
	else {
		firsttime = true;
		RedrawSurface(m);
	}
}

void MouseMove(GlobalParams* m, bool isCalledWhenMouseAcuallyMoved){
	if (isCalledWhenMouseAcuallyMoved) {
		if (abs(m->TransferWASDMoveMomentiumXIntArithmetic) > 0.01f || abs(m->TransferWASDMoveMomentiumYIntArithmetic) > 0.01f) {
			return;
		}
	}
	bool isAMouseButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_MBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
	if ((!isAMouseButtonDown)&& test1) {
		MouseUp(m);
		test1 = false;
	}
	//auto start = std::chrono::high_resolution_clock::now();
		
	ShowCursor(1);
	POINT pos = { 0 };
	GetCursorPos(&pos);

	// i would probably min this
	if (m->isInCropMode) {
		ScreenToClient(m->hwnd, &pos);
		HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
		uint32_t distLeft, distRight, distTop, distBottom;
		GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

		uint32_t range = 20;
		//if(pos.x < 50){


		if (pos.x < (distLeft + range) && pos.x >(distLeft - range) && pos.y < (distTop + range) && pos.y >(distTop - range)) {
			m->CropHandleSelectTL = true;
			m->CropHandleSelectTR = false;
			m->CropHandleSelectBL = false;
			m->CropHandleSelectBR = false;
		} else if (pos.x < (distRight + range) && pos.x >(distRight - range) && pos.y < (distTop + range) && pos.y >(distTop - range)) {
			m->CropHandleSelectTL = false;
			m->CropHandleSelectTR = true;
			m->CropHandleSelectBL = false;
			m->CropHandleSelectBR = false;
		} else if (pos.x < (distLeft + range) && pos.x >(distLeft - range) && pos.y < (distBottom + range) && pos.y >(distBottom - range)) {
			m->CropHandleSelectTL = false;
			m->CropHandleSelectTR = false;
			m->CropHandleSelectBL = true;
			m->CropHandleSelectBR = false;
		} else if (pos.x < (distRight + range) && pos.x >(distRight - range) && pos.y < (distBottom + range) && pos.y >(distBottom - range)) {
			m->CropHandleSelectTL = false;
			m->CropHandleSelectTR = false;
			m->CropHandleSelectBL = false;
			m->CropHandleSelectBR = true;
		}
		else {
			m->CropHandleSelectTL = false;
			m->CropHandleSelectTR = false;
			m->CropHandleSelectBL = false;
			m->CropHandleSelectBR = false;
		}

		if (m->isMovingTL) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, pos.x, pos.y, &perX, &perY);
			m->leftP = perX;
			m->topP = perY;
			if (m->leftP > m->rightP) { m->leftP = m->rightP; }
			if (m->topP > m->bottomP) { m->topP = m->bottomP; }
		}

		if (m->isMovingTR) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, pos.x, pos.y, &perX, &perY);
			m->rightP = perX;
			m->topP = perY;
			if (m->rightP < m->leftP) { m->rightP = m->leftP; }
			if (m->topP > m->bottomP) { m->topP = m->bottomP; }
		}

		if (m->isMovingBL) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, pos.x, pos.y, &perX, &perY);
			m->leftP = perX;
			m->bottomP = perY;
			if (m->leftP > m->rightP) { m->leftP = m->rightP; }
			if (m->bottomP < m->topP) { m->bottomP = m->topP; }
		}

		if (m->isMovingBR) {
			float perX, perY;
			GetCropPercentagesFromCursor(m, pos.x, pos.y, &perX, &perY);
			m->rightP = perX;
			m->bottomP = perY;
			if (m->rightP < m->leftP) { m->rightP = m->leftP; }
			if (m->bottomP < m->topP) { m->bottomP = m->topP; }
		}

		RedrawSurface(m);

		SetCursor(cursor);
		ShowCursor(TRUE);
		return;
	}

	if (m->slider1mousedown) {
		ScreenToClient(m->hwnd, &pos);
		float findMid = (float)(pos.x - (m->drawMenuOffsetX + m->slider1begin)) / (float)(m->slider1end-m->slider1begin);
		m->testfloat = findMid;
		if (findMid > 0.0f) {
			m->drawSize = pow((findMid*10.0f),2)+1;
			if (m->drawSize < 1.0f) { m->drawSize = 1.0f; }
		}
		else {
			m->drawSize = 1.0f;
		}
		RedrawSurface(m);

	}
	else if (m->slider2mousedown) {
		ScreenToClient(m->hwnd, &pos);
		float findMid = (float)(pos.x - (m->drawMenuOffsetX + m->slider2begin)) / (float)(m->slider2end - m->slider2begin);
		m->testfloat = findMid;
		if (findMid >= 0.0f && findMid <= 1.0f) {
			m->a_opacity = findMid;
			//if (m->a_opacity < 0.01f) { m->a_opacity = 0.01f; }
		}
		else if (findMid < 0) {
			m->a_opacity = 0;
		}
		else {
			m->a_opacity = 1.0f;
		}
		RedrawSurface(m);
	}
	else if (m->drawmousedown) {
		placeDraw(m, &pos);
		GuidedRedrawSurface(m);
		return;
	}
	else if (m->mouseDown) {

		// Moving around the image using left mouse button

		
		m->iLocX = m->lockimgoffx - (m->LockmPos.x - pos.x);
		m->iLocY = m->lockimgoffy - (m->LockmPos.y - pos.y);
		
		GuidedRedrawSurface(m);
		
	}
	else {
		ScreenToClient(m->hwnd, &pos);

		if (pos.y <= m->toolheight) {
			m->lock = true;
			m->selectedbutton = getXbuttonID(m, pos);

			HRGN rgn = CreateRectRgn(0, 0, m->width, m->toolheight+24);
			SelectClipRgn(m->hdc, rgn);
			RedrawSurface(m, false, true);

			DeleteObject(rgn);
		}
		else {
			if (m->lock) {
				m->selectedbutton = -1;
				RedrawSurface(m);
				m->lock = false;
			}
		}
	}

	
	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	bool isInMenu = IfInMenu(pos, m) && m->isMenuState;
	bool isInImage = IsInImage(pos, m);

	bool as = false;
	if (m->drawmode && isInImage && !isInMenu) {
		cursor = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_CURSOR1));
		//cursor = CreateCircleCursor(m->drawSize*m->mscaler);
		as = true;
	}

	if (m->eyedroppermode) {
		cursor = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_CURSOR2));
	}

	
	if (m->isMenuState) {
		HRGN rgn = CreateRectRgn(m->actmenuX, m->actmenuY, m->actmenuX + m->menuSX, m->actmenuY + m->menuSY);
		SelectClipRgn(m->hdc, rgn);
		RedrawSurface(m,false, true);

		DeleteObject(rgn);
	}


	SetCursor(cursor);
	ShowCursor(TRUE);
	//auto end = std::chrono::high_resolution_clock::now();
		//std::chrono::duration<float, std::milli> duration = end - start;
		//m->etime = duration.count();
	
}
GlobalParams* m_proc;
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT umsg, WPARAM wparam, LPARAM lparam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		SetTimer(hwndDlg, 1, 100, NULL); // Set a timer to check the condition periodically
		HWND i = GetDlgItem(hwndDlg, IDC_PROGRESS1);
		SendMessage(i, (WM_USER + 10), (WPARAM)TRUE, (LPARAM)10);
		return TRUE;
	}
	case WM_TIMER: {
		if (m_proc->ProcessOfMakingUndoStep == 0) {
			EndDialog(hwndDlg, 0); // Close the dialog when the condition is false

		}
		return TRUE;
	}
	case WM_CLOSE:
		EndDialog(hwndDlg, 0); // Handle the close button
		return TRUE;
	}
	return FALSE;
}



// Function to show a modal dialog box while processing
void showMessageWhileProcessing(GlobalParams* m) {
	
	m_proc = m;
	m->tint = true;
	RedrawSurface(m);
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(LoadingHalt), m->hwnd, DialogProc);
	m->tint = false;
	RedrawSurface(m);
}

uint32_t genrand() {
	std::uint32_t min = 0;
	std::uint32_t max = 4294967295;

	// Initialize random number generation
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<std::uint32_t> dis(min, max);

	// Generate a random number
	std::uint32_t random_number = dis(gen);
	return random_number;
}

bool goodbye(GlobalParams* m, uint32_t id) {
	std::string path = m->undofolder + std::to_string(id) + "-ViewImage.bmp";
	if (!DeleteFile(path.c_str())) {
		MessageBox(m->hwnd, "Error cleaning undo data packages", "ERROR", MB_OK | MB_ICONERROR);
		return 0;
	}
	return 1;
}

uint32_t* hello(GlobalParams* m, uint32_t id) {
	std::string path = m->undofolder + std::to_string(id) + "-ViewImage.bmp";
	int x, y, c;
	uint32_t* data = (uint32_t*)stbi_load(path.c_str(), &x, &y, &c, 4);
	return data;
}

bool classUndo = true;
void PushUndo(GlobalParams* m, uint32_t* thisImage, uint32_t* thisOImage) {

	m->ProcessOfMakingUndoStep++;
	
	for (int y = m->undoData.size(); y > m->undoStep; y--) {
		auto back = m->undoData.back();
		int last = back.imageID;
		int lasto = back.imageIDoriginal;
		if (!goodbye(m, last)) { m->ProcessOfMakingUndoStep = 0; return; }
		if (!goodbye(m, lasto)) { m->ProcessOfMakingUndoStep = 0; return; }
		m->undoData.pop_back();
	}

	uint32_t imageID = genrand();
	uint32_t imageIDo = genrand();

	std::string path1 = m->undofolder + std::to_string(imageID)  + "-ViewImage.bmp";
	bool is = stbi_write_bmp(path1.c_str(), m->imgwidth, m->imgheight, 4, thisImage);

	std::string path2 = m->undofolder + std::to_string(imageIDo) + "-ViewImage.bmp";
	bool is2 = stbi_write_bmp(path2.c_str(), m->imgwidth, m->imgheight, 4, thisOImage);

	if (!is) {
		MessageBox(m->hwnd, "Failed to create undo data package", "Oops", MB_OK | MB_ICONERROR);
		return;
	}

	if (!is2) {
		MessageBox(m->hwnd, "Failed to create undo data package (Original Image)", "Oops", MB_OK | MB_ICONERROR);
		return;
	}

	m->undoStep++;
	UndoDataStruct k;
	k.imageID = imageID;
	k.imageIDoriginal = imageIDo;
	k.height = m->imgheight;
	k.width = m->imgwidth;
	m->undoData.push_back(k);

	FreeData(thisImage);
	FreeData(thisOImage);

	classUndo = true;
	m->ProcessOfMakingUndoStep--;
}

void createUndoStep(GlobalParams* m, bool async) {
	if (!async) {
		if (m->ProcessOfMakingUndoStep > 0) {
			m->loading = true;
			RedrawSurface(m);
			showMessageWhileProcessing(m);
		}
	}

	uint32_t* thisImage = (uint32_t*)malloc(m->imgwidth * m->imgheight * 4);
	if (!thisImage) {
		MessageBox(m->hwnd, "The Undo Step Failed", "Undo Step Fail", MB_OK | MB_ICONERROR);
		exit(0);
	}

	uint32_t* thisOImage = (uint32_t*)malloc(m->imgwidth * m->imgheight * 4);
	if (!thisOImage) {
		MessageBox(m->hwnd, "The Undo Step Failed", "Undo Step Fail", MB_OK | MB_ICONERROR);
		exit(0);
	}

	memcpy(thisImage, (uint32_t*)m->imgdata, m->imgwidth * m->imgheight * 4);
	memcpy(thisOImage, (uint32_t*)m->imgoriginaldata, m->imgwidth * m->imgheight * 4);

	//PushUndo(m, thisImage, thisOImage);
	if (async) {
		std::thread t{ PushUndo, m, thisImage, thisOImage };
		t.detach();
	}
	else {
		PushUndo(m, thisImage, thisOImage);
	}

	m->loading = false;
}



void UndoBus(GlobalParams* m) {
	if (m->drawmousedown) {
		return;
	}
	if (m->ProcessOfMakingUndoStep > 0) {
		m->loading = true;
		RedrawSurface(m);
		showMessageWhileProcessing(m);
	}

	if (classUndo) { createUndoStep(m, false); m->undoStep--; }
    if (m->undoStep > 0) {
        m->undoStep--;
        UndoDataStruct selection = m->undoData[m->undoStep];
		FreeData(m->imgdata);
		FreeData(m->imgoriginaldata);
		m->imgdata = malloc(selection.width * selection.height * 4);
		m->imgoriginaldata = malloc(selection.width * selection.height * 4);
		m->imgwidth = selection.width;
		m->imgheight = selection.height;
		uint32_t* d1 = hello(m, selection.imageID);
		uint32_t* d2 = hello(m, selection.imageIDoriginal);
		if (!d1) {
			MessageBox(m->hwnd, "Annotated undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			m->loading = false;
			return;
		}
		if (!d2) {
			MessageBox(m->hwnd, "RAW undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			m->loading = false;
			return;
		}

        memcpy(m->imgdata, d1, selection.width * selection.height * 4);
		memcpy(m->imgoriginaldata, d2, selection.width * selection.height * 4);
		FreeData(d1);
		FreeData(d2);
    }
	classUndo = false;
	m->loading = false;
	RedrawSurface(m);
}

void RedoBus(GlobalParams* m) {
	if (m->drawmousedown) {
		return;
	}
	if (m->ProcessOfMakingUndoStep > 0) {
		m->loading = true;
		RedrawSurface(m);
		showMessageWhileProcessing(m);
	}

	int s = m->undoStep;
	int step = m->undoData.size() - 1;
	if (s < step) {
		m->undoStep++;
		UndoDataStruct selection = m->undoData[m->undoStep];
		FreeData(m->imgdata);
		FreeData(m->imgoriginaldata);
		m->imgdata = malloc(selection.width * selection.height * 4);
		m->imgoriginaldata = malloc(selection.width * selection.height * 4);
		m->imgwidth = selection.width;
		m->imgheight = selection.height;
		uint32_t* d1 = hello(m, selection.imageID);
		uint32_t* d2 = hello(m, selection.imageIDoriginal);
		if (!d1) {
			MessageBox(m->hwnd, "Annotated undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			m->loading = false;
			return;
		}
		if (!d2) {
			MessageBox(m->hwnd, "RAW undo data package failed to load", "Oops", MB_OK | MB_ICONERROR);
			m->loading = false;
			return;
		}
		memcpy(m->imgdata, d1, selection.width * selection.height * 4);
		memcpy(m->imgoriginaldata, d2, selection.width * selection.height * 4);
		FreeData(d1);
		FreeData(d2);
	}
	classUndo = false;
	m->loading = false;
	RedrawSurface(m);
	
}

void ToggleFullscreen(GlobalParams* m) { 
	if (m->fullscreen) {
		// disable fullscreen
		SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		m->fullscreen = false; // to fix fullscreen "testing" issue with toolheight
		SetWindowPlacement(m->hwnd, &m->wpPrev);
		SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		
		RedrawSurface(m);
	}
	else {
		m->fullscreen = true;
		int screenX = GetSystemMetrics(SM_CXSCREEN);
		int screenY = GetSystemMetrics(SM_CYSCREEN);
		GetWindowPlacement(m->hwnd, &m->wpPrev);
		SetWindowLong(m->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(m->hwnd, 0, 0, 0, screenX, screenY, 0);
		RedrawSurface(m);
	}
}

void zoomcycle(GlobalParams* m, float factor) {
	for (int i = 0; i < 10; i++) { // Use the "for" loop to recursively make more perfect to reduce rounding errors
		float factor_s = factor / m->mscaler;
		NewZoom(m, factor_s, true, false);
		RedrawSurface(m);
	}
}

// This was a really stupid idea
/*
void CheckKeys(GlobalParams* m, WPARAM wparam) {
	// Check if the pressed key is W, A, S, or D
	if (wparam == 'W' || wparam == 'A' || wparam == 'S' || wparam == 'D') {
			LARGE_INTEGER frequency, previousTime, currentTime;
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&previousTime);

			// Continuously check the key states as long as any of them are pressed
			while (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
				MSG msg = { 0 };
				while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				QueryPerformanceCounter(&currentTime);
				double deltaTime = static_cast<double>(currentTime.QuadPart - previousTime.QuadPart) / frequency.QuadPart;
				previousTime = currentTime;

				HWND activeWindow = GetActiveWindow();
				if (activeWindow == m->hwnd) {
					// Check and update for each key individually
					if (GetAsyncKeyState('W') & 0x8000) {
						//Beep(800, 50);
						m->iLocY += static_cast<int>(deltaTime * 1000);
					}
					if (GetAsyncKeyState('S') & 0x8000) {
						//Beep(1600, 50);
						m->iLocY -= static_cast<int>(deltaTime * 1000);
					}
					if (GetAsyncKeyState('A') & 0x8000) {
						//Beep(400, 50);
						m->iLocX += static_cast<int>(deltaTime * 1000);
					}
					if (GetAsyncKeyState('D') & 0x8000) {
						//Beep(200, 50);
						m->iLocX -= static_cast<int>(deltaTime * 1000);
					}
					RedrawSurface(m);
				}
			}
		// Clear the message queue events
		MSG msg;
		while (PeekMessage(&msg, nullptr, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE)) {}
	}

}
*/


void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam) {


	if (m->halt) { return; }

	HWND temp = GetActiveWindow();
	if (temp != m->hwnd) {
		return;
	}

	if (m->fullscreen) {
		while (ShowCursor(FALSE) >= 0); // Hide idle cursor in fullscreen
	}

	if ((GetKeyState(VK_ESCAPE) & 0x8000)) {
		m->eyedroppermode = false;
		m->isInCropMode = false;
		RedrawSurface(m);
	}

	if (((GetKeyState(VK_SHIFT) & 0x8000) && wparam == 'Z')&& m->drawmode) { // Eyedropper
		m->eyedroppermode = true;
		RedrawSurface(m);
	}

	if (((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) && wparam == 'D') {
		m->debugmode = !m->debugmode;
	}


	// FOR KEYBOARD NAVIGATION
	if (((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) && (m->imgwidth >= 1)) { // ALSO FOUND IN REDRAW SURFACE TO DRAW TXT
		RedrawSurface(m);
		//Beep(1000, 400);
		if (wparam >= '1' && wparam <= '9'){
			PerformCasedBasedOperation(m, wparam - 0x31, false);
		}
		if (wparam == '0') {
			PerformCasedBasedOperation(m, 9, false);
		}
		if (wparam == VK_OEM_MINUS) {
			PerformCasedBasedOperation(m, 10, false);
		}
		return;
	}



	if (wparam == VK_F11) {
		ToggleFullscreen(m);
	}
	if (wparam == VK_ESCAPE) {
		if (m->fullscreen) {
			// disable fullscreen
			SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

			SetWindowPlacement(m->hwnd, &m->wpPrev);
			SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
			m->fullscreen = false;
			RedrawSurface(m);
		}
	}


	if (wparam == 'F' && !m->isInCropMode) {
		PrepareOpenImage(m);
	}


	if (m->imgwidth < 1) {
		return;
	}


	if (wparam == VK_OEM_PLUS) { // WHY? (the plus sign or equals sign)
		NewZoom(m, 1.25f, 2, true);
	}
	if (wparam == VK_OEM_MINUS) { // WHY? (the minus sign or dash sign)
		NewZoom(m, 0.8f, 2, true);

	}

	if (wparam == VK_LEFT) {
		GoLeft(m);
	}
	if (wparam == VK_RIGHT) {
		GoRight(m);
	}

	if (wparam == '1') {
		zoomcycle(m, 1.0f);
	}

	if (wparam == '2') {
		zoomcycle(m, 2.0f);
	}
	if (wparam == '3') {
		zoomcycle(m, 0.5f);
	}

	if (wparam == '4') {
		zoomcycle(m, 4.0f);
	}

	if (wparam == '5') {
		autozoom(m);
		RedrawSurface(m);
	}

	if (wparam == '6') {
		zoomcycle(m, 0.25f);
	}

	if (wparam == '7') {
		zoomcycle(m, 0.125f);
	}

	if (wparam == '8') {
		//m->mscaler = 8.0f;
		zoomcycle(m, 8.0f);
	}


	if (m->isInCropMode) {
		return;
	}

	if (wparam == 'Z' && GetKeyState(VK_CONTROL) & 0x8000) {
		// undo
		UndoBus(m);
		RedrawSurface(m);
	}

	if(wparam == 'C' && GetKeyState(VK_CONTROL) & 0x8000) {
		// copy
		if (!CopyImageToClipboard(m, m->imgdata, m->imgwidth, m->imgheight)) {
			MessageBox(m->hwnd, "Error copying the image to the clipboard. You used control+c.", "Bug Detected!", MB_OK | MB_ICONERROR);
		}
	}

	if (wparam == 'V' && GetKeyState(VK_CONTROL) & 0x8000) {
		// paste NEXT VER 2.5 COMING UP
		/*
		
		if (!PasteImageFromClipboard(m)) {
			MessageBox(m->hwnd, "Error pasting the image from the clipboard. You used control+v.", "Bug Detected!", MB_OK | MB_ICONERROR);
		}
		*/
	}

	if (wparam == 'S' && GetKeyState(VK_CONTROL) & 0x8000) {
		// save
		PrepareSaveImage(m);
	}

	if (wparam == 'U' && GetKeyState(VK_CONTROL) & 0x8000 && GetKeyState(VK_SPACE) & 0x8000) {
		// undo
		m->pinkTestCenter = true;
		RedrawSurface(m);

		m->pinkTestCenter = false;
	}
	if (wparam == 'R' && GetKeyState(VK_CONTROL) & 0x8000) {
		// resize
		ShowResizeDialog(m);
	}
	if (wparam == 'Y' && GetKeyState(VK_CONTROL) & 0x8000) {
		// undo
		RedoBus(m);
		RedrawSurface(m);
	}



	/*
	* // weird blur thing
	if (wparam == 'H' && GetKeyState(VK_CONTROL) & 0x8000) {
		for (int y = 1000; y > 50; y -= 2) {
			ResizeImageToSize(m, y,y);
		}
		
		RedrawSurface(m);
	}
	*/
	
	
	if (wparam == 'G') {
		m->drawmode = !m->drawmode;
		RedrawSurface(m);
	}

	//CheckKeys(m, wparam);

	if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
		// dont do it
	}
	else {
		MouseMove(m);
	}
}

void MouseUp(GlobalParams* m) {
	if (m->isInCropMode) {
		m->isMovingTL = false;
		m->isMovingTR = false;
		m->isMovingBL = false;
		m->isMovingBR = false;
		return;
	}
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(m->hwnd, &pos);
	if (m->eyedroppermode) {
		// eyedropper here
		bool smooth = m->smoothing;
		m->smoothing = false;
		RedrawSurface(m);
		uint32_t color = *GetMemoryLocation(m->scrdata, pos.x, pos.y, m->width, m->height);
		m->a_drawColor = color;

		m->eyedroppermode = false;
		MouseMove(m);
		m->smoothing = smooth;
		RedrawSurface(m);

		return;
	}

	m->isSize = false;
	m->mouseDown = false;
	m->toolmouseDown = false;
	TurnOffDraw(m);

	m->slider1mousedown = false;
	m->slider2mousedown = false;
	m->slider3mousedown = false;
	m->slider4mousedown = false; 


	if (m->isMenuState) {
		// menu logic
		if (m->IsLastMouseDownWhenOverMenu) { // prevent sliding mouse error (especially on effects)
			if (IfInMenu(pos, m)) { 

				int selected = (pos.y - (m->actmenuY + 2)) / m->mH;
				if (selected < m->menuVector.size()) {
					auto l = m->menuVector[selected].func;
					if (l) {
						l();
						m->isMenuState = false;
					}
				}

				//Beep(selected *100, 100);
			}
		}
		
	}
	RedrawSurface(m);
}
bool aot = false;
void RightDown(GlobalParams* m) {
	if (m->isInCropMode) {
		return;
	}
	if (m->eyedroppermode) {
		return;
	}

	m->lastMouseX = -1;
	m->lastMouseY = -1;
	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);

	m->mouseRightInitialCheckX = mPP.x;
	m->mouseRightInitialCheckY = mPP.y;

	if (IfInMenu(mPP, m) && m->isMenuState) { // check if the mouse is over a menu
		return;
	}

	CloseMenuWhenInactive(m, mPP);

	if (m->drawmode) {
		if (IsInImage(mPP, m)) {// NOW, im putting it in the right click menu up thing to not rendeeer menu ACTUALLLY its right down below
			m->drawtype = 0;
			TurnOnDraw(m);
			createUndoStep(m, true);
		}
	}
}


void RightUp(GlobalParams* m) {
	if (m->isInCropMode) {
		ConfirmCrop(m);
		return;
	}
	if (m->eyedroppermode) {
		return;
	}
	bool isdraw = m->drawmousedown;
	TurnOffDraw(m);
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(m->hwnd, &pos);
	if (isdraw) {
		return;
	}
	if (pos.y < m->toolheight) {
		return;
	}
	if (m->drawmode) {
		if (IsInImage(pos, m)) {
			return;
		}
	}
	if(!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)){
		m->menuVector = {

			{"Blank Image",
				[m]() -> bool {
					m->isMenuState = false;
					if (AllocateBlankImage(m, 0xFFFFFFFF)) {
						ShowResizeDialog(m);
					}
					return true;
				},0,0
			},

			{"Toggle Smoothing{s}",
				[m]() -> bool {
					m->isMenuState = false;
					m->smoothing = !m->smoothing;
					return true;
				},13,0
			},

			{"Undo (CTRL+Z)",
				[m]() -> bool {
					m->isMenuState = false;
					UndoBus(m);
					return true;
				},26,0
			},

			{"Redo (CTRL+Y){s}",
				[m]() -> bool {
					m->isMenuState = false;
					RedoBus(m);
					return true;
				},39,0
			},

			{"Resize Image [CTRL+R]",
				[m]() -> bool {
					m->isMenuState = false;
					RedrawSurface(m);
					ShowResizeDialog(m);
					//ResizeImageToSize(m);
					return true;
				},52,0
			},
			{"Information",
				[m]() -> bool {
					ShowMyInformation(m);
					return true;
				},13,13
			},

			{"Set Always On Top",
				[m]() -> bool {
					if (aot) {
						SetWindowPos(m->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						aot = false;
					}
					else {
						SetWindowPos(m->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						aot = true;
					}
					return true;
				},26,13
			},

		};
		if (aot) {
			m->menuVector[6].atlasX = 39;
		}
		m->menuX = pos.x;
		m->menuY = pos.y;
		m->isMenuState = true;
		RedrawSurface(m);
	}
}



void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam) {

	if (m->imgwidth < 1) return;
	float zDelta = (float)GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);


	int slider1begin = m->drawMenuOffsetX + m->slider1begin;
	int slider1end = m->drawMenuOffsetX + m->slider1end;

	int slider2begin = m->drawMenuOffsetX + m->slider2begin;
	int slider2end = m->drawMenuOffsetX + m->slider2end;

	int sliderYb = m->drawMenuOffsetY;
	int sliderYe = m->drawMenuOffsetY + 40; // PLEASE STOP COPYING THINGS AROUND AND MAKE A FUNCTION!! nah ill do it later
	if (m->drawmode) {

		if (CheckIfMouseInSlider1(mPP, m, slider1begin, slider1end, sliderYb, sliderYe)) { // these are also found in the scroll USE CONTROL F
			if (zDelta > 0) {
				m->drawSize += 1.0f;
				m->drawSize *= 1.3f;
			}
			else {
				m->drawSize -= 1.0f;
				m->drawSize *= 0.76f;
			}
			if (m->drawSize < 1) { m->drawSize = 1; }
			if (m->drawSize > 100.0f) { m->drawSize = 100.0f; }
			RedrawSurface(m);
			return;
		}

		if (CheckIfMouseInSlider2(mPP, m, slider2begin, slider2end, sliderYb, sliderYe)) {
			if (zDelta > 0) {
				m->a_opacity += 0.1f;
			}
			else {
				m->a_opacity -= 0.1f;
			}
			if (m->a_opacity < 0.01f) { m->a_opacity = 0.01f; }
			if (m->a_opacity > 1.0f) { m->a_opacity = 1.0f; }
			RedrawSurface(m);
			return;
		}
	}
	
	// CONTROL F!



	float v = 1;

	if (zDelta > 0 && m->mscaler < 400.0f) {
		v = 1.25f;
	}
	else if (m->mscaler > 0.01f) {
		v = 0.8f;
	}

	if ((GetKeyState(VK_MENU) & 0x8000)&&m->drawmode) {// why do they call the alt key VK_MENU
		m->drawSize *= v;
		RedrawSurface(m);
	}
	else {
		NewZoom(m, v, true, true);
	}

}
 