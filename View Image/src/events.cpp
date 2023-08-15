#include "headers/events.h"
#include "../resource.h"
#include <Shlwapi.h>

bool Initialization(GlobalParams* m, int argc, LPWSTR* argv) {


	char buffer[MAX_PATH];
	DWORD length = GetModuleFileName(nullptr, buffer, MAX_PATH);
	PathRemoveFileSpec(buffer);

	m->cd = std::string(buffer);

	//InitFont(hwnd, "C:\\Windows\\Fonts\\segoeui.ttf", 14);
	m->scrdata = malloc(m->width * m->height * 4);

	m->toolbarData = LoadImageFromResource(IDB_PNG1, m->widthos, m->heightos, m->channelos);

	// turn arguments into path
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, NULL, 0, NULL, NULL);


	InitFont(m->hwnd, "C:\\Windows\\Fonts\\segoeui.ttf", 14);

	char* path = (char*)malloc(size_needed);
	WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, path, size_needed, NULL, NULL);
	
	
	if (argc >= 2) {
		if (!OpenImageFromPath(m, path)) {
			MessageBox(m->hwnd, "Unable to open image", "Error", MB_OK);
			exit(0);
			return false;
		}
	}
	else {
		RedrawImageOnBitmap(m);
	}
	
	return true;
}

bool ToolbarMouseDown(GlobalParams* m) {
	POINT mPP;
	GetCursorPos(&mPP);
	ScreenToClient(m->hwnd, &mPP);
	uint32_t id = getXbuttonID(m, mPP);
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
		RedrawImageOnBitmap(m);

		return 0;
	case 6:
		// rotate

		// rotate = later
		rotateImage90Degrees(m);
		//OpenImageFromPath(m, cpath.c_str());

		//MessageBox(hwnd, "I haven't added this feature yet", "Can't rotate image", MB_OK);
		return 0;
	case 7: {



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
		return 0;
	}
	case 8: {

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
	m->lockimgoffx = m->iLocX;
	m->lockimgoffy = m->iLocY;
	GetCursorPos(&m->LockmPos);
	POINT k;
	GetCursorPos(&k);
	ScreenToClient(m->hwnd, &k);
	if (k.y > m->toolheight) {
		m->mouseDown = true;
	}
}

void MouseMove(GlobalParams* m) {
	POINT pos = { 0 };
	GetCursorPos(&pos);
	if (m->mouseDown) {

		if (GetKeyState('G') & 0x8000) {
			ScreenToClient(m->hwnd, &pos);
			int k = (int)((float)(pos.x - m->CoordLeft) * (1.0f / m->mscaler));
			int v = (int)((float)(pos.y - m->CoordTop) * (1.0f / m->mscaler));
			// drawing
			int size = (float)m->imgheight * 0.05f;
			int halfsize = size / 2;

			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++) {
					double yes = pow(x - halfsize, 2) + pow(y - halfsize, 2);
					if (yes <= pow(halfsize, 2)) {

						uint32_t xloc = (x - halfsize) + k;
						uint32_t yloc = (y - halfsize) + v;
						if (xloc < m->imgwidth && yloc < m->imgheight && xloc >= 0 && yloc >= 0) {
							uint32_t* memoryPath = GetMemoryLocation(m->imgdata, xloc, yloc, m->imgwidth);
							*memoryPath = lerp(0xFFFF0000, *memoryPath, yes / pow(halfsize, 2)); // transparency
						}
					}
				}
			}

			RedrawImageOnBitmap(m);
			return;
		}
		else {

			m->iLocX = m->lockimgoffx - (m->LockmPos.x - pos.x);
			m->iLocY = m->lockimgoffy - (m->LockmPos.y - pos.y);

		}

		RedrawImageOnBitmap(m);
	}
	else {

		ScreenToClient(m->hwnd, &pos);

		if (pos.y <= m->toolheight) {
			m->lock = true;
			m->selectedbutton = getXbuttonID(m, pos);
			RedrawImageOnBitmap(m);
		}
		else {
			if (m->lock) {
				m->selectedbutton = -1;
				RedrawImageOnBitmap(m);
				m->lock = false;
			}
		}
	}

	RedrawImageOnBitmap(m);
}

void KeyDown(GlobalParams* m, WPARAM wparam, LPARAM lparam) {
	if (m->halt) { return; }

	HWND temp = GetActiveWindow();
	if (temp != m->hwnd) {
		return;
	}
	if (wparam == 'G') {

	}
	if (wparam == VK_F11) {
		if (m->fullscreen) {
			// disable fullscreen
			SetWindowLong(m->hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

			SetWindowPlacement(m->hwnd, &m->wpPrev);
			SetWindowPos(m->hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
			m->fullscreen = false;
			RedrawImageOnBitmap(m);
		}
		else {
			SetWindowLong(m->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
			int screenX = GetSystemMetrics(SM_CXSCREEN);
			int screenY = GetSystemMetrics(SM_CYSCREEN);
			GetWindowPlacement(m->hwnd, &m->wpPrev);
			SetWindowPos(m->hwnd, 0, 0, 0, screenX, screenY, 0);
			m->fullscreen = true;
			RedrawImageOnBitmap(m);
		}
	}
	if (wparam == VK_RIGHT) {
		

		const char* mpath = m->fpath.c_str();
		
		std::string k = GetNextFilePath(mpath);
		const char* npath = k.c_str();
		//MessageBox(0, mpath, npath, MB_OKCANCEL);
		if (k != "No") {
			OpenImageFromPath(m, npath);
		}		

	}
	if (wparam == 'F') {
		PrepareOpenImage(m);
	}

	if (wparam == '1') {
		m->mscaler = 1.0f;
	}

	if (wparam == '2') {
		m->mscaler = 2.0f;
	}
	if (wparam == '3') {
		m->mscaler = 0.5f;
	}

	if (wparam == '4') {
		m->mscaler = 4.0f;
	}

	if (wparam == '5') {
		autozoom(m);
	}

	if (wparam == '6') {
		m->mscaler = 0.25f;
	}

	if (wparam == '7') {
		m->mscaler = 0.125f;
	}

	if (wparam == '8') {
		m->mscaler = 8.0f;
	}
	RedrawImageOnBitmap(m);
}

void MouseUp(GlobalParams* m) {
	m->isSize = false;
	m->mouseDown = false;
}

void Size(GlobalParams* m) {
	if (m->scrdata) {

		RECT ws = { 0 };
		GetClientRect(m->hwnd, &ws);
		m->width = ws.right - ws.left;
		m->height = ws.bottom - ws.top;


		free(m->scrdata);

		m->scrdata = malloc(m->width * m->height * 4);

		RedrawImageOnBitmap(m);
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


	NewZoom(m, v, true);
}