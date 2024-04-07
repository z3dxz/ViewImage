#include "headers/events.h" // e
#include "../resource.h"
#include <Shlwapi.h>
#include <vector>

void createUndoStep(GlobalParams* m);

void ToggleFullscreen(GlobalParams* m);

bool Initialization(GlobalParams* m, int argc, LPWSTR* argv) {


	char buffer[MAX_PATH];
	DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
	PathRemoveFileSpec(buffer);

	m->cd = std::string(buffer);

	//InitFont(hwnd, "C:\\Windows\\Fonts\\segoeui.TTF", 14);
	size_t scrsize = m->width * m->height * 4;
	m->scrdata = malloc(scrsize);
	for (uint32_t i = 0; i < m->width * m->height; i++) {
		*((uint32_t*)m->scrdata+i) = 0x008080;

	}

	m->toolbar_gaussian_data = malloc(m->width * m->toolheight * 4);

	m->toolbarData_shadow = LoadImageFromResource(IDB_PNG2, m->widthos, m->heightos, m->channelos);
	m->toolbarData = LoadImageFromResource(IDB_PNG5, m->widthos, m->heightos, m->channelos);
	


	int null1, null2, null3;
	m->mainToolbarCorner = LoadImageFromResource(IDB_PNG3, null1, null2, null3);

	m->fullscreenIconData = LoadImageFromResource(IDB_PNG4, null1, null2, null3);

	// turn arguments into path
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);


	InitFont(m, "C:\\Windows\\Fonts\\segoeui.TTF", 14);

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

	Size(m);
	
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

bool ToolbarMouseDown(GlobalParams* m) {
	if (m->isMenuState) {
		return 1;
	}
	m->toolmouseDown = true;
	if (m->drawmode) {
		POINT mPP;
		GetCursorPos(&mPP);
		ScreenToClient(m->hwnd, &mPP);
		if (mPP.y > m->toolheight && mPP.x >= m->CoordLeft && mPP.y > m->CoordTop && mPP.x < m->CoordRight && mPP.y < m->CoordBottom) {
			m->drawmousedown = true;
			createUndoStep(m);
		}
	}

	// add toolbar clicky here
	{
		POINT mp;
		GetCursorPos(&mp);
		ScreenToClient(m->hwnd, &mp);
		// fullscreen icon
		if ((mp.x > m->width - 36 && mp.x < m->width - 13) && (mp.y > 12 && mp.y < 33)) { //fullscreen icon location check coordinates (ALWAYS KEEP)
			ToggleFullscreen(m); // TODO: please make a seperate icon for the exiting fullscreen

			return 0;
		}
	}

	if(m->drawmode)
	{
		POINT mp;
		GetCursorPos(&mp);
		ScreenToClient(m->hwnd, &mp);
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
		int sliderYe = m->drawMenuOffsetY+40;



		if ((mp.x > colorBeginX && mp.x < colorEndX) && (mp.y > colorBeginY && mp.y < colorEndY)) { //color icon coordinates
			// open color picker
			bool success = true;
			uint32_t c = PickColorFromDialog(m, m->a_drawColor, &success);
			if (success) {
				m->a_drawColor = c;
			}
			return 0;
		}

		//  I copied these to the rendering function so that I can make a hover effect
		if ((mp.x > slider1begin && mp.x < slider1end) && (mp.y > sliderYb && mp.y < sliderYe)) {
			m->slider1mousedown = true;
		}

		if ((mp.x > slider2begin && mp.x < slider2end) && (mp.y > sliderYb && mp.y < sliderYe)) {
			m->slider2mousedown = true;
		}

	}

	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);
	uint32_t id = getXbuttonID(m, mPP);

	

	if (m->imgwidth > 0 || id == 0) {  } else { return 1; }
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
		NewZoom(m, 1.25f, 2);
		return 0;
	case 3:
		// zoom out
		NewZoom(m, 0.8f, 2);
		return 0;
	case 4:
		// zoom fit
		autozoom(m);
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

		if (m->fpath != "Untitled") {

			// delete
			int result = MessageBox(m->hwnd, "This will delete the image permanently!!!", "Are You Sure?", MB_YESNO);
			if (result == IDYES) {
				DeleteFile(m->fpath.c_str());
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
	case 9: {
		// print
		Print(m);
		
		return 0;
	}
	case 10: {

		// image info

		char str[256];

		std::string txt = "No Image Loaded";
		if (!m->fpath.empty()) {
			txt = m->fpath;
		}
		sprintf(str, "Image Information:\n%s\nWidth: %d\nHeight: %d\n\nAbout:\nMade for cosine64.com",txt.c_str(), m->imgwidth, m->imgheight);

		MessageBox(m->hwnd, str, "Information", MB_OK);
		return 0;
	}
	}

	return 1;
}

void MouseDown(GlobalParams* m) {


	m->lastMouseX = -1;
	m->lastMouseY = -1;
	m->lockimgoffx = m->iLocX;
	m->lockimgoffy = m->iLocY;
	GetCursorPos(&m->LockmPos);
	POINT k;
	GetCursorPos(&k);
	ScreenToClient(m->hwnd, &k);


	if (k.x > m->menuX && k.y > m->menuY && k.x < (m->menuX + m->menuSX) && k.y < (m->menuY + m->menuSY)) {

	}
	else {
		
		m->isMenuState = false;
		RedrawSurface(m);
	}

	if (k.y > m->toolheight) {
		if(!m->isMenuState)
		m->mouseDown = true;
	}
}


POINT* sampleLine(GlobalParams* m, double x1, double y1, double x2, double y2, int* outSamples) {
	float sizex = abs(x2 - x1);
	float sizey = abs(y2 - y1);
	float dist = (sqrt(pow(sizex, 2) + pow(sizey, 2)));
	int numSamples = (dist/(m->drawSize/4))+2;

	POINT* samples = (POINT*)malloc(sizeof(POINT) * numSamples);

	for (int i = 0; i < numSamples; i++) {
		double t = (double)i / (numSamples - 1);
		samples[i].x = x1 + t * (x2 - x1);
		samples[i].y = y1 + t * (y2 - y1);
	}

	// remember to deallocate
	*outSamples = numSamples;
	return samples;
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

uint32_t change_alpha(uint32_t color, uint8_t new_alpha) {
	// Assuming color format is 0xRRGGBBAA
	return (color & 0xFFFFFF) | (static_cast<uint32_t>(new_alpha) << 24);
}


void placeDraw(GlobalParams* m, POINT* pos) {


	ScreenToClient(m->hwnd, pos);

	int k = (int)((float)((m->lastMouseX) - m->CoordLeft) * (1.0f / m->mscaler));
	int v = (int)((float)((m->lastMouseY) - m->CoordTop) * (1.0f / m->mscaler));

	int k1 = (int)((float)(pos->x - m->CoordLeft) * (1.0f / m->mscaler));
	int v1 = (int)((float)(pos->y - m->CoordTop) * (1.0f / m->mscaler));

	if (m->lastMouseX == -1) { k = k1; v = v1; }

	//int samples0 = m->a_hardness;

	int samples0;
	POINT* k2 = sampleLine(m, k1, v1, k, v, &samples0);

	for (int i = 0; i < samples0; i++) {

		// drawing here!
		int diameter = m->drawSize;
		float radius = (float)diameter / 2.0f;

		for (int y = 0; y < diameter; y++) {
			for (int x = 0; x < diameter; x++) {
				float virtual_x = (float)x + 0.5f;
				float virtual_y = (float)y + 0.5f;
				double distance = sqrt(pow(virtual_x - radius, 2) + pow(virtual_y - radius, 2)); // pythagorean theorem AKA circle formula
				if (distance > radius) { distance = radius; }

				uint32_t xloc = (virtual_x - (radius)) + (k2[i].x);
				uint32_t yloc = (virtual_y - (radius)) + (k2[i].y);
				//xloc += (rand() % m->a_frost) - m->a_frost / 2;
				//yloc += (rand() % m->a_frost) - m->a_frost / 2;
				if (xloc < m->imgwidth && yloc < m->imgheight && xloc >= 0 && yloc >= 0) {
					// drawing
					uint32_t* memoryPath = GetMemoryLocation(m->imgdata, xloc, yloc, m->imgwidth, m->imgheight);
					*memoryPath = lerp(*memoryPath, change_alpha(m->a_drawColor, 255), (1.0f-(distance / radius)) * m->a_opacity); // transparency
					m->shouldSaveShutdown = true;
				}
				
			}
		}
	}

	free(k2);

	m->lastMouseX = pos->x;
	m->lastMouseY = pos->y;


	RedrawSurface(m);
}


bool beforeas = false;

void MouseMove(GlobalParams* m) {
	ShowCursor(1);
	POINT pos = { 0 };
	GetCursorPos(&pos);

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
			return;
	}
	else if (m->mouseDown) {

		m->iLocX = m->lockimgoffx - (m->LockmPos.x - pos.x);
		m->iLocY = m->lockimgoffy - (m->LockmPos.y - pos.y);

		RedrawSurface(m);
		
	}
	else {

		ScreenToClient(m->hwnd, &pos);

		if (pos.y <= m->toolheight) {
			m->lock = true;
			m->selectedbutton = getXbuttonID(m, pos);
			HRGN rgn = CreateRectRgn(0, 0, m->width, m->toolheight+24);
			SelectClipRgn(m->hdc, rgn);
			RedrawSurface(m);
			SelectClipRgn(m->hdc, NULL);

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
	bool isInMenu = ((pos.x > m->menuX && pos.y > m->menuY && pos.x < (m->menuX + m->menuSX) && pos.y < (m->menuY + m->menuSY))) && m->isMenuState;
	bool isInImage = pos.y > m->toolheight && pos.x >= m->CoordLeft && pos.y >= m->CoordTop && pos.x < m->CoordRight && pos.y < m->CoordBottom;

	bool as = false;
	if (m->drawmode && isInImage && !isInMenu) {
		cursor = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_CURSOR1));
		as = true;
	}
	m->isAnnotationCircleShown = as;
	if (beforeas != as) {
		RedrawSurface(m);
	}

	/*
#pragma region Cusor
	if (m->drawmode && as) {
		int redrawmargins = 20;

		// first
		int d = m->mscaler * m->drawSize;
		int r = d / 2;

		int locationx = -redrawmargins - (int)(r)+m->lastMoveX;
		int locationy = -redrawmargins - (int)(r)+m->lastMoveY;
		int sizex = d + redrawmargins * 2;
		int sizey = d + redrawmargins * 2;
		HRGN rgn = CreateRectRgn(locationx, locationy, locationx + sizex, locationy + sizey);
		SelectClipRgn(m->hdc, rgn);
		RedrawSurface(m);
		SelectClipRgn(m->hdc, NULL);
		DeleteObject(rgn);
		// second
		{

			int d = m->mscaler * m->drawSize;
			int r = d / 2;

			int locationx = -redrawmargins - (int)(r)+pos.x;
			int locationy = -redrawmargins - (int)(r)+pos.y;
			int sizex = d + redrawmargins * 2;
			int sizey = d + redrawmargins * 2;
			HRGN rgn = CreateRectRgn(locationx, locationy, locationx + sizex, locationy + sizey);
			SelectClipRgn(m->hdc, rgn);
			RedrawSurface(m);
			SelectClipRgn(m->hdc, NULL);
			DeleteObject(rgn);
		}
	}

#pragma endregion
	*/
	if (m->isMenuState) {
		HRGN rgn = CreateRectRgn(m->menuX, m->menuY, m->menuX + m->menuSX, m->menuY + m->menuSY);
		SelectClipRgn(m->hdc, rgn);
		RedrawSurface(m);
		SelectClipRgn(m->hdc, NULL);

		DeleteObject(rgn);
	}


	SetCursor(cursor);
	ShowCursor(TRUE);
	m->lastMoveX = pos.x;
	m->lastMoveY = pos.y;
	beforeas = as;

	
}

bool classUndo = true;
void createUndoStep(GlobalParams* m) {
    uint32_t* thisImage = (uint32_t*)malloc(m->imgwidth * m->imgheight * 4);
    if (!thisImage) {
        MessageBox(m->hwnd, "The Undo Step Failed", "Undo Step Fail", MB_OK | MB_ICONERROR);
        exit(0);
    }
    memcpy(thisImage, (uint32_t*)m->imgdata, m->imgwidth * m->imgheight * 4);
	for (int y = m->undoData.size(); y > m->undoStep; y--) {
		uint32_t* last = m->undoData.back().image;
		free(last);
		m->undoData.pop_back();
	}
	m->undoStep++;
	UndoDataStruct k;
	k.image = thisImage;
	k.height = m->imgheight;
	k.width = m->imgwidth;
    m->undoData.push_back(k);
	classUndo = true;
}

void UndoBus(GlobalParams* m) {
	if (classUndo) { createUndoStep(m); m->undoStep--; }
    if (m->undoStep > 0) {
        m->undoStep--;
        UndoDataStruct selection = m->undoData[m->undoStep];
		free(m->imgdata);
		m->imgdata = malloc(selection.width * selection.height * 4);
		m->imgwidth = selection.width;
		m->imgheight = selection.height;
        memcpy(m->imgdata, selection.image, selection.width * selection.height * 4);
    }
	classUndo = false;
}

void RedoBus(GlobalParams* m) {
	int s = m->undoStep;
	int step = m->undoData.size() - 1;
	if (s < step) {
		m->undoStep++;
		UndoDataStruct selection = m->undoData[m->undoStep];
		free(m->imgdata);
		m->imgdata = malloc(selection.width * selection.height * 4);
		m->imgwidth = selection.width;
		m->imgheight = selection.height;
		memcpy(m->imgdata, selection.image, selection.width * selection.height * 4);
	}
	classUndo = false;
}

void ToggleFullscreen(GlobalParams* m) {
	if (m->fullscreen) {
		// disable fullscreen
		SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

		SetWindowPlacement(m->hwnd, &m->wpPrev);
		SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		m->fullscreen = false;
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

void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam) {

	if (m->halt) { return; }

	HWND temp = GetActiveWindow();
	if (temp != m->hwnd) {
		return;
	}
	if (m->fullscreen) {
		while (ShowCursor(FALSE) >= 0); // Hide idle cursor in fullscreen
	}

	if (wparam == 'F') {
		PrepareOpenImage(m);
		RedrawSurface(m);
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

	if (m->imgwidth < 1) {
		return;
	}

	if (wparam == 'Z' && GetKeyState(VK_CONTROL) & 0x8000) {
		// undo
		UndoBus(m);
		RedrawSurface(m);
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

	if (wparam == VK_LEFT) {
		if (!m->loading && !m->halt && m->imgwidth>0) {
			m->halt = true;
			m->loading = true;

			std::string k = GetPrevFilePath();
			const char* npath = k.c_str();
			//MessageBox(0, mpath, npath, MB_OKCANCEL);
			if (k != "No") {
				OpenImageFromPath(m, npath, true);
			}
			m->loading = false;
			m->halt = false;
		}
		RedrawSurface(m);
	}
	if (wparam == VK_RIGHT) {
		if (!m->loading && !m->halt && m->imgwidth > 0) {
			m->halt = true;
			m->loading = true;

			const char* mpath = m->fpath.c_str();

			std::string k = GetNextFilePath(mpath);
			const char* npath = k.c_str();
			//MessageBox(0, mpath, npath, MB_OKCANCEL);
			if (k != "No") {
				OpenImageFromPath(m, npath, true);
			}
			m->loading = false;
			m->halt = false;
		}
		RedrawSurface(m);
	}

	if (wparam == '1') {
		m->mscaler = 1.0f;
		RedrawSurface(m);
	}

	if (wparam == '2') {
		m->mscaler = 2.0f;
		RedrawSurface(m);
	}
	if (wparam == '3') {
		m->mscaler = 0.5f;
		RedrawSurface(m);
	}

	if (wparam == '4') {
		m->mscaler = 4.0f;
		RedrawSurface(m);
	}

	if (wparam == '5') {
		autozoom(m);
		RedrawSurface(m);
	}

	if (wparam == '6') {
		m->mscaler = 0.25f;
		RedrawSurface(m);
	}

	if (wparam == '7') {
		m->mscaler = 0.125f;
		RedrawSurface(m);
	}

	if (wparam == '8') {
		m->mscaler = 8.0f;
		RedrawSurface(m);
	}
}

void MouseUp(GlobalParams* m) {
	m->isSize = false;
	m->mouseDown = false;
	m->toolmouseDown = false;
	m->drawmousedown = false;

	m->slider1mousedown = false;
	m->slider2mousedown = false;
	m->slider3mousedown = false;
	m->slider4mousedown = false;

	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(m->hwnd, &pos);

	if (m->isMenuState) {
		// menu logic
		if (pos.x > m->menuX && pos.y > m->menuY && pos.x < (m->menuX + m->menuSX) && pos.y < (m->menuY + m->menuSY)) {

			int selected = (pos.y - (m->menuY + 2)) / m->mH;
			auto l = m->menuVector[selected].second;
			l();
			m->isMenuState = false;
			RedrawSurface(m);
			//Beep(selected *100, 100);
		}
	}
}

void RightUp(GlobalParams* m) {
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(m->hwnd, &pos);
		m->menuVector = {

			{"Blank Image{s}",
				[m]() -> bool {
					if (AllocateBlankImage(m, 0xFFFFFFFF)) {
						ShowResizeDialog(m);
					}
					return true;
				},
			},

			{"Toggle Smoothing{s}",
				[m]() -> bool {
					m->smoothing = !m->smoothing;
					return true;
				},
			},

			{"Undo (CTRL+Z)",
				[m]() -> bool {
					UndoBus(m);
					return true;
				},
			},

			{"Redo (CTRL+Y){s}",
				[m]() -> bool {
					RedoBus(m);
					return true;
				},
			},

			{"Resize Image [CTRL+R]",
				[m]() -> bool {
					ShowResizeDialog(m);
					//ResizeImageToSize(m);
					return true;
				},
			},
		
		};
		m->menuX = (pos.x > (m->width-m->menuSX)) ? (m->width - m->menuSX) : pos.x;
		m->menuY = (pos.y > (m->height - m->menuSY)) ? (m->height - m->menuSY) : pos.y;
		m->isMenuState = true;

		RedrawSurface(m);
		//SelectClipRgn(m->hdc, NULL);
}

void Size(GlobalParams* m) {
	if (m->scrdata) {

		RECT ws = { 0 };
		GetClientRect(m->hwnd, &ws);
		m->width = ws.right - ws.left;
		m->height = ws.bottom - ws.top;

		if (m->width < 1) { m->width = 1; }
		if (m->height < 1) { m->height = 1; }

		m->scrdata = realloc(m->scrdata, m->width * m->height * 4);
		m->toolbar_gaussian_data = realloc(m->toolbar_gaussian_data, m->width * m->toolheight * 4);

		m->ith.resize(m->width);
		m->itv.resize(m->height);

		for (uint32_t i = 0; i < m->width; i++)
			m->ith[i] = i;
		for (uint32_t i = 0; i < m->height; i++)
			m->itv[i] = i;

		autozoom(m);

		RedrawSurface(m);
	}
}

void MouseWheel(GlobalParams* m, WPARAM wparam, LPARAM lparam) {
	



	if (m->imgwidth < 1) return;
	float zDelta = (float)GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;

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
		NewZoom(m, v, true);
	}

}
