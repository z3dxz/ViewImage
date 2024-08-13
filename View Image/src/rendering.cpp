
#include "headers/rendering.hpp"
#include <vector>
#include <thread>
#include <time.h>
#include <shlwapi.h>
#include <wincodec.h>

// Freetype Globals
FT_Library ft;
FT_Face* currentFace;


#define CanRenderToolbarMacro (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight)&&!m->isInCropMode)
void CircleGenerator(int circleDiameter, int locX, int locY, int outlineThickness, int smoothening, uint32_t* backgroundBuffer, uint32_t foregroundColor, float opacity, int backgroundWidth, int backgroundHeight) {
	float circleD = (float)circleDiameter;

	float circleR = circleD / 2;

	float smoothenings = (float)smoothening;

	float circleBrushSize = (outlineThickness);

	int margin = 5;
	float margins = margin;
	for (int y = 0; y < circleD + margins; y++) {
		for (int x = 0; x < circleD + margins; x++) {
			
			float solution = pow(x - circleR - margins, 2) + pow(y - circleR - margins, 2);

			float color = remap(solution, 0, pow(circleR, 2), 0, 1);

			float min0 = remap(circleD - smoothenings, 0, circleD, 0, 1);
			float max0 = remap(circleD, 0, circleD, 0, 1);

			float min1 = remap(circleD - smoothenings - circleBrushSize, 0, circleD, 0, 1);
			float max1 = remap(circleD - circleBrushSize, 0, circleD, 0, 1);

			float color_outside = remap(color, min0, max0, 0, 1);
			float color_inside = remap(color, min1, max1, 0, 1);

			float fcolor = color_inside - color_outside;

			int pixelX = x - margin - (int)circleR + locX;
			int pixelY = y - margin - (int)circleR + locY;
			if ((pixelX >= 0 && pixelX < backgroundWidth) && (pixelY >= 0 && pixelY < backgroundHeight)) {
				uint32_t* backgroundLocation = GetMemoryLocation(backgroundBuffer, pixelX, pixelY, backgroundWidth, backgroundHeight);
				*backgroundLocation = lerp(*backgroundLocation, foregroundColor, (fcolor*opacity));
			}
		}
	}
}

std::string GetWindowsFontsFolder() {
	HKEY hKey;
	char fontPath[MAX_PATH];
	DWORD size = MAX_PATH;

	// Open the registry key where the fonts directory path is stored
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Query the "FontDir" value
		if (RegQueryValueEx(hKey, "FontDir", nullptr, nullptr, (LPBYTE)fontPath, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return std::string(fontPath);
		}
		RegCloseKey(hKey);
	}

	// Fallback: Use the standard Windows directory + "Fonts" if registry lookup fails
	if (GetWindowsDirectory(fontPath, MAX_PATH)) {
		PathAppend(fontPath, "Fonts");
		return std::string(fontPath);
	}

	// If everything fails, return an empty string
	return "Fail";
}

void initfontfolder(GlobalParams* m) {
	m->fontsfolder = GetWindowsFontsFolder();
	if (m->fontsfolder == "Fail") {
		MessageBox(m->hwnd, "Fonts directory not found\n", "Error", MB_OK | MB_ICONERROR);
	}
	
}

bool alreadyInit = false;
FT_Face LoadFont(GlobalParams* m, std::string fontA) {
	if (!alreadyInit) {
		if (FT_Init_FreeType(&ft)) {
			MessageBox(m->hwnd, "Failed to initialize FreeType library\n", "Error", MB_OK | MB_ICONERROR);
			return 0;
		}
		alreadyInit = true;
	}
	
	if (m->fontsfolder == "") {
		initfontfolder(m);
	}
	std::string font_s = (m->fontsfolder + "\\" + fontA);
	FT_Face k;
	if (FT_New_Face(ft, font_s.c_str(), 0, &k)) {
		std::string er = "Failed to load font: " + fontA + "\nYou may need to install this on your system";
		MessageBox(m->hwnd, er.c_str(), "Error Loading Font", MB_OK | MB_ICONERROR);
		return 0;
	}
	return k;
}

/*
bool InitFont(GlobalParams* m, std::string fontA, int size) {
	
	if (FT_Init_FreeType(&ft)) {
		MessageBox(m->hwnd, "Failed to initialize FreeType library\n", "Error", MB_OK | MB_ICONERROR);
		m->fontinit = false;
		return false;
	}
	if (m->fontsfolder == "") {
		initfontfolder(m);
	}
	std::string font_s = (m->fontsfolder + "\\" + fontA);
	
	if (FT_New_Face(ft, font_s.c_str(), 0, &face)) {
		if (!isalreadyopened) {
			std::string er = "Failed to load font: " + fontA;
			MessageBox(m->hwnd, er.c_str(), "Error Loading Font", MB_OK | MB_ICONERROR);
			isalreadyopened = true;
		}
		m->fontinit = false;
		return false;
	}

	FT_Set_Pixel_Sizes(face, 0, size);
	m->fontinit = true;
	return true;
}
*/


/*

const char* strtable[] {
	"Open Image (F)",
	"Save as PNG (CTRL+S)",
	"Zoom In",
	"Zoom Out",
	"Zoom Auto",
	"Zoom 1:1 (100%)",
	"Rotate",
	"Annotate (G)",
	"DELETE image",
	"Print",
	"Copy Image",
	"Information"
};


*/
void SwitchFont(FT_Face& font) {
	currentFace = &font;
}

int ActuallyPlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	if (!*currentFace) {
		return 0;
	}
	FT_Set_Pixel_Sizes(*currentFace, 0, size);

	if (bufwidth < 100) return 0;
	const char* text = inputstr;

	FT_GlyphSlot g = (*currentFace)->glyph;
	int penX = 0;
	int spacing = 0; // Variable to track spacing between characters

	// Calculate the base Y-position using ascender value
	int baseY = locY + ((*currentFace)->size->metrics.ascender >> 6);

	for (const char* p = text; *p; ++p) {
		if (FT_Load_Char((*currentFace), *p, FT_LOAD_RENDER))
			continue;

		FT_Bitmap* bitmap = &g->bitmap;

		int maxGlyphHeight = 0;
		int maxDescender = 0;
		
		int yOffset = (g->metrics.horiBearingY - g->bitmap_top) >> 6;

		if (g->bitmap.rows > maxGlyphHeight)
			maxGlyphHeight = g->bitmap.rows;

		int descender = (g->metrics.height >> 6) - yOffset;
		if (descender > maxDescender)
			maxDescender = descender;

		for (int y = 0; y < bitmap->rows; ++y) {
			for (int x = 0; x < bitmap->width; ++x) {
				int pixelIndex = (y)*bufwidth + (penX + x);
				unsigned char pixelValue = bitmap->buffer[y * bitmap->width + x];
				uint32_t ptx = locX + penX + x;
				uint32_t pty = baseY - (bitmap->rows - y) + maxDescender; // Adjusted Y-position calculation

				uint32_t* memoryPath = GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight);
				uint32_t* memoryPathFrom = GetMemoryLocation(fromBuffer, ptx, pty, bufwidth, bufheight);
				uint32_t existingColor = *memoryPathFrom;
				if (ptx >= 0 && pty >= 0 && ptx < bufwidth && pty < bufheight) {
					*GetMemoryLocation(mem, ptx, pty, bufwidth, bufheight) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
				}
			}
		}

		// Calculate actual character width
		int characterWidth = (g->metrics.horiAdvance >> 6) - (g->metrics.horiBearingX >> 6);

		// Update penX position with character width and spacing
		penX += characterWidth + spacing;
	}

	return 1;
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem) {
	bool e = ActuallyPlaceString(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);
	return e;
}

int PlaceString(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int bufwidth, int bufheight, void* fromBuffer) {
	bool e = ActuallyPlaceString(m, size, inputstr, locX, locY, color, mem, bufwidth, bufheight, fromBuffer);
	return e;
}

int PlaceStringShadow(GlobalParams* m, int size, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem, int shadowY, int shadowX = 0) {
	bool b = ActuallyPlaceString(m, size, inputstr, locX+shadowX, locY + shadowY, 0x000000, mem, m->width, m->height, mem);
	bool e = ActuallyPlaceString(m, size, inputstr, locX, locY, color, mem, m->width, m->height, mem);
	return e && b;
}




void dDrawRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				int realX = xloc + x;
				int realY = yloc + y;

				if (realX >= 0 && realY >= 0 && realX < kwidth && realY < kheight) {
					uint32_t* ma = GetMemoryLocation(mem, realX, realY, kwidth, kheight);
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
		}
	}
}

void dDrawRoundedFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int realX = xloc + x;
			int realY = yloc + y;

			if (realX >= 0 && realY >= 0 && realX < kwidth && realY < kheight) {
				uint32_t* ma = GetMemoryLocation(mem, realX, realY, kwidth, kheight);
				//if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1) && !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
			//}
		}
	}
}

void dDrawFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if ((xloc + x) < 0 || (xloc + x) >= kwidth || (yloc + y) < 0 || (yloc + y) >= kheight) {
				continue;
			}
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth, kheight);
			if (opacity < 0.99f) {
				*ma = lerp(*ma, color, opacity);
			}
			else {
				*ma = color;
			}
		}
	}
}
 
void dDrawRoundedRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if ((xloc + x) < 0 || (xloc + x) >= kwidth || (yloc + y) < 0 || (yloc + y) >= kheight) {
				continue;
			}
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth, kheight);
			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1)&& !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			}
		}
	}
}

// MULTITHREADING!!!!!
// https://www.youtube.com/watch?v=46ddlUImiQA
//*********************************************


void PlaceImageNN(GlobalParams* m, void* memory, bool invert, POINT p) {

	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth = m->imgwidth;
	const int32_t imgheight = m->imgheight;
	const int margin = 2;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {

			uint32_t bkc = 0x111111;
			// bkc
			if (y > m->toolheight || !(CanRenderToolbarMacro)) {

				if (((x / 9) + (y / 9)) % 2 == 0) {
					bkc = 0x121212;
				}
				else {
					bkc = 0x0C0C0C;
				}
			}

			int32_t offX = (((int32_t)m->width - (int32_t)(imgwidth * m->mscaler)) / 2) + m->iLocX;
			int32_t offY = (((int32_t)m->height - (int32_t)(imgheight * m->mscaler)) / 2) + m->iLocY;

			int32_t ptx = (x - offX) * inv_mscaler;
			int32_t pty = (y - offY) * inv_mscaler;

			uint32_t doColor = bkc;
			if (ptx < imgwidth && pty < imgheight && ptx >= 0 && pty >= 0 &&
				x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) { // check if image is within it's coordinates
				uint32_t c = *GetMemoryLocation(memory, ptx, pty, imgwidth, imgheight);

				int alpha = (c >> 24) & 255;
				if (alpha == 255) {
					doColor = c;
				}
				else if (alpha == 0) {
					// still bkc
				}
				else {
					doColor = lerp(bkc, c, static_cast<float>(alpha) / 255.0f);
				}
			}
			*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
		}
		});
}

bool followsPattern(int number) {
	double logBase2 = log2(number / 100.0);
	return logBase2 == floor(logBase2);
}



void PlaceImageBI(GlobalParams* m, void* memory, bool invert, POINT p) {


	//auto start = std::chrono::high_resolution_clock::now();

	bool doesfollow = false;
	if (followsPattern(m->mscaler * 100)) {
		doesfollow = true;
	}

	const int margin = m->fullscreen ? 0 : 2;
	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth_minus_1 = m->imgwidth - 1;
	const int32_t imgheight_minus_1 = m->imgheight - 1;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {

			uint32_t bkc = 0x111111;
			// bkc
			if (y > m->toolheight || !(CanRenderToolbarMacro)) {

				if (((x / 9) + (y / 9)) % 2 == 0) {
					bkc = 0x121212;
				}
				else {
					bkc = 0x0C0C0C;
				}
			}

			int32_t offX = (((int32_t)m->width - (int32_t)(m->imgwidth * m->mscaler)) / 2) + m->iLocX;
			int32_t offY = (((int32_t)m->height - (int32_t)(m->imgheight * m->mscaler)) / 2) + m->iLocY;

			float ptx = (x - offX) * inv_mscaler - 0.5f;
			float pty = (y - offY) * inv_mscaler - 0.5f;

			if (doesfollow) {
				ptx = (x - offX) * inv_mscaler;
				pty = (y - offY) * inv_mscaler;
			}

			uint32_t doColor = bkc;

			if (ptx < imgwidth_minus_1 && pty < imgheight_minus_1 && ptx >= 0 && pty >= 0 && x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) {
				int32_t ptx_int = static_cast<int32_t>(ptx);
				int32_t pty_int = static_cast<int32_t>(pty);
				float ptx_frac = ptx - ptx_int;
				float pty_frac = pty - pty_int;

				// Get the four nearest pixels
				uint32_t c00 = *GetMemoryLocation(memory, ptx_int, pty_int, m->imgwidth, m->imgheight);
				uint32_t c01 = *GetMemoryLocation(memory, ptx_int + 1, pty_int, m->imgwidth, m->imgheight);
				uint32_t c10 = *GetMemoryLocation(memory, ptx_int, pty_int + 1, m->imgwidth, m->imgheight);
				uint32_t c11 = *GetMemoryLocation(memory, ptx_int + 1, pty_int + 1, m->imgwidth, m->imgheight);
				
				if (((c00 >> 24) & 0xFF) != 0 || ((c01 >> 24) & 0xFF) != 0 || ((c10 >> 24) & 0xFF) != 0 || ((c11 >> 24) & 0xFF) != 0) {
					uint32_t placeColor = lerp(lerp(c00, c01, ptx_frac), lerp(c10, c11, ptx_frac), pty_frac);

					int alpha = (placeColor >> 24) & 255;
					if (alpha == 255) {
						doColor = placeColor;
					}
					else if (alpha == 0) {
						// still bkc
					}
					else {
						doColor = lerp(bkc, placeColor, static_cast<float>(alpha) / 255.0f);
					}
				}
			}
			*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
		}
	});

	//auto end = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<float, std::milli> duration = end - start;
	//float etime = duration.count();
	//m->etime = etime;
}



void RenderToolbarIcon(GlobalParams* m, int index, int locationX, void* data, uint32_t color, uint32_t selectedColor) {
	ToolbarButtonItem* item = &m->toolbartable[index];
	for (uint32_t y = 0; y < m->iconSize; y++) {
		for (uint32_t x = 0; x < m->iconSize; x++) {

			int locationOnBitmap = item->indexX;

			uint32_t l = (*GetMemoryLocation(data, x+locationOnBitmap, y, m->widthos, m->heightos));
			float alphaM = (float)((l >> 24) & 0xFF) / 255.0f;
			float valueM = (float)((l >> 16) & 0xFF) / 255.0f;
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, locationX + x, 6 + y, m->width, m->height);

			uint32_t targetColor = color;
			if (index == m->selectedbutton) {
				targetColor = selectedColor;
			}

			

			*memoryPath = lerp(*memoryPath, multiplyColor(targetColor, valueM), alphaM);
		}
	}
	if (item->isSeperator) {
		int location = locationX + (m->iconSize - 1)+5;
		for (int y = 0; y < m->toolheight-24; y++) {
			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, location, y + 12, m->width, m->height);
			*memoryPath = lerp(*memoryPath, 0xFFFFFF, 0.3f);
		}
	}
}

void RenderToolbarButtons(GlobalParams* m, void* data, uint32_t color, uint32_t selectedColor) {
	int p = 5;
	for (size_t i = 0; i < m->toolbartable.size(); ++i) {
		RenderToolbarIcon(m, i, p, data, color, selectedColor);
		//p += m->iconSize + 5 + (m->toolbartable[i].isSeperator * 4);
		p += GetIndividualButtonPush(m, i);
		if (m->imgwidth < 1) {
			return;
		}
		if (m->drawmode && i == 11) {
			return;
		}
	}
}

void RenderMainToolbarButtons(GlobalParams* m, void* data, int maxButtons, uint32_t color, float opacity, uint32_t selectedColor, float selectedOpacity) {
	int p = 5;
	int k = 0;

	for (int i = 0; i < maxButtons; i++) {
		if ((p) > (m->width - (m->iconSize))) continue;
		for (uint32_t y = 0; y < m->iconSize; y++) {
			for (uint32_t x = 0; x < m->iconSize; x++) {

				uint32_t l = (*GetMemoryLocation(data, x + k, y, m->widthos, m->heightos));
				float alphaM = (float)((l >> 24) & 0xFF)/255.0f;
				float valueM = (float)((l >> 16) & 0xFF)/255.0f;
				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, p + x, 6 + y, m->width, m->height);

				uint32_t targetColor = color;
				float targetOpacity = opacity;
				if (i == m->selectedbutton) {
					targetColor = selectedColor;
					targetOpacity = selectedOpacity;
				}

				*memoryPath = lerp(*memoryPath, multiplyColor(targetColor, valueM), alphaM * targetOpacity);

			}
		}
		p += m->iconSize + 5; k += m->iconSize + 1;
	}
}
/*

void RenderToolbarButtonShadow(GlobalParams* m, void* data, int maxButtons, uint32_t color, float opacity, uint32_t selectedColor, float selectedOpacity) {
	int p = 5;
	int k = 0;

	for (int i = 0; i < maxButtons; i++) {
		if ((p) > (m->width - (m->iconSize))) continue;
		for (uint32_t y = 0; y < m->iconSize; y++) {
			for (uint32_t x = 0; x < m->iconSize; x++) {


				uint32_t l = 255-(*GetMemoryLocation(data, x + k, y, m->widthos, m->heightos));
				int l0 = ((l & 0x00FF0000) >> 16);
				float ll = (float)l0 / 255.0f;
				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, p + x, 6 + y, m->width, m->height);
					if (i != m->selectedbutton) {
						*memoryPath = lerp(*memoryPath, color, ll * opacity); // transparency
					}
					else {
						*memoryPath = lerp(*memoryPath, selectedColor, ll * selectedOpacity); // transparency
					}
			}
		}
		p += m->iconSize + 5; k += m->iconSize + 1;
	}
}

*/
void RenderToolbar(GlobalParams* m) {
		
		
		// Render the toolbar
		// 
		//BLUR FOR TOOLBAR
		
		if (m->CoordTop <= m->toolheight) {
			gaussian_blur_toolbar(m, (uint32_t*)m->scrdata);
		}

		// TOOLBAR CONTAINER
		for (uint32_t y = 0; y < m->toolheight; y++) {
			for (uint32_t x = 0; x < m->width; x++) {
				uint32_t color = 0x050505;
				float opacity = 0.65f;
				if (y == m->toolheight - 2) { color = 0x303030; }
				if (y == m->toolheight - 1) { color = 0x000000; opacity = 1.0f; }

				if (y == 0) color = 0x303030;
				if (y == 1) color = 0x000000;
				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
				*memoryPath = lerp(*memoryPath, color, opacity); // transparency
			}
		}
		
		

		// BUTTONS 
		if (!m->toolbarData) return;

		// render toolbar buttons shadow

		//int mybutton = m->maxButtons;
		//if (m->imgwidth < 1) mybutton = 1;

		//RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x606060, 0.7f, 0x606060, 0.7f);
		//RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x000000, 0.7f, 0, 0.7f);
		RenderToolbarButtons(m, m->toolbarData, 0xFFFFFF, 0xFFE0E0);


		// version
		SwitchFont(m->OCRAExt);
		if (m->width > 540) {
			PlaceString(m, 13, REAL_BIG_VERSION_BOOLEAN, m->width - 72, 16, 0x909090, m->scrdata);
		}
		else {
			PlaceString(m, 13, REAL_BIG_VERSION_BOOLEAN, m->width - 50, m->toolheight + 4, 0x909090, m->scrdata);
		}

		// tooltips AND OUTLINE FOR THE BUTTONS (basically stuff when its selected)
		
		
		//-- The border when selecting annotate
		if (m->drawmode) {
			// 7 means draw
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, 7) + 2), 4, m->iconSize+5, m->toolheight - 8, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, 7) + 2) - 1, 3, m->iconSize+5 + 2, m->toolheight - 6, 0x000000, 1.0f);
		}

		if (m->selectedbutton >= 0 && m->selectedbutton < m->toolbartable.size()) {
			// rounded corners: split hover thing into three things
			int in = m->selectedbutton;

			//-- The fill when selecting the buttons
			dDrawFilledRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, in) + 2) + 1, 4, m->iconSize+5 - 2, 1, 0xFF8080, 0.2f);
			dDrawFilledRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, in) + 2), 5, m->iconSize+5, m->toolheight - 10, 0xFF8080, 0.2f);
			dDrawFilledRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, in) + 2) + 1, (m->toolheight - 5), m->iconSize - 2, 1, 0xFF8080, 0.2f);
			
			//-- The outline wb border when selecting the buttons
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, in) +2), 4, m->iconSize+5, m->toolheight - 8, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (GetLocationFromButton(m, in) + 2)-1, 3, m->iconSize+5 +2, m->toolheight - 6, 0x000000, 1.0f);

			

			std::string txt = "Error";
			if (m->selectedbutton < m->toolbartable.size()) {
				txt = m->toolbartable[m->selectedbutton].name;
			}

			// tooltips
			if (!m->isMenuState) {
				int loc = 1 + (GetLocationFromButton(m, in) + 2);
				dDrawFilledRectangle(m->scrdata, m->width, m->height, loc, m->toolheight + 5, (txt.length() * 8) + 10, 18, 0x000000, 0.8f);
				dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0x808080, 0.4f);

				SwitchFont(m->OCRAExt);
				PlaceString(m, 14, txt.c_str(), loc + 4, m->toolheight + 4, 0xFFFFFF, m->scrdata);
			}
			
		}
		

		// fullscreen icon
		if (m->width > 535) {
			POINT mp;
			GetCursorPos(&mp);
			ScreenToClient(m->hwnd, &mp);
			bool nearf = false;
			if ((mp.x > m->width - 36 && mp.x < m->width - 13) && (mp.y > 12 && mp.y < 33)) { //fullscreen icon location check coordinates (ALWAYS KEEP)
				nearf = true;
			}

			if(nearf) {
				//-- The outline for the fullscreen button
				dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->width-36, 12, 24, 23, 0xFFFFFF, 0.2f);
				dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->width-37, 11, 26, 25, 0x000000, 1.0f);

				SwitchFont(m->Tahoma);
				//-- Fullscreen icon tooltip
				if (m->fullscreen) {
					PlaceString(m, 13, "Exit Fullscreen (F11)", m->width - 120, 48, 0xFFFFFF, m->scrdata);
				}
				else {
					PlaceString(m, 13, "Fullscreen (F11)", m->width - 100, 48, 0xFFFFFF, m->scrdata);
				}

			}
			for (int y = 0; y < 11; y++) {
				for (int x = 0; x < 12; x++) {
					uint32_t* oLoc = GetMemoryLocation(m->scrdata, m->width + x - 30, y + 18, m->width, m->height);
					float t = 0.5f;
					if (nearf) {
						t = 1.0f;
					}
					float transparency = ((float)((*GetMemoryLocation(m->fullscreenIconData, x, y, 12, 11)) & 0xFF) / 255.0f) * t;
					*oLoc = lerp(*oLoc, 0xFFFFFF, transparency);
				}
			}
		}
		

}

void RenderToolbarShadow(GlobalParams *m) {
	// drop shadow for the toolbar (not buttons)
	for (uint32_t y = 0; y < 20; y++) {
		for (uint32_t x = 0; x < m->width; x++) {
			uint32_t color = 0x000000;

			uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y + m->toolheight, m->width, m->height);
			*memoryPath = lerp(*memoryPath, color, (1.0f - ((float)y / 20.0f)) * 0.3f); // transparency
		}
	}
}

void ResetCoordinates(GlobalParams* m) {

	if (m->imgwidth > 0) {

		m->CoordLeft = (m->width / 2) - (-m->iLocX) - (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordTop = (m->height / 2) - (-m->iLocY) - (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

		m->CoordRight = (m->width / 2) - (-m->iLocX) + (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordBottom = (m->height / 2) - (-m->iLocY) + (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

	}
	else {

		m->CoordLeft = 0;
		m->CoordTop = 0;

		m->CoordRight = 0;
		m->CoordBottom = 0;
	}
}

void DrawMenuIcon(GlobalParams* m, int locationX, int locationY, int atlasX, int atlasY) {
	// 12x12
	int iconSize = 12;
	for (int y = 0; y < iconSize; y++) {
		for (int x = 0; x < iconSize; x++) {
			uint32_t* inAtlas = GetMemoryLocation(m->menu_icon_atlas, atlasX + x, atlasY + y, m->menu_atlas_SizeX, m->menu_atlas_SizeY);
			uint32_t* inMem = GetMemoryLocation(m->scrdata, locationX + x, locationY + y, m->width, m->height);
			float alphaM = (float)((*inAtlas >> 24) & 0xFF) / 255.0f;
			//float valueM = (float)((*inAtlas >> 16) & 0xFF) / 255.0f;
			*inMem = lerp(*inMem, 0xFF6060, alphaM);
		}
	}
}

void DrawMenu(GlobalParams* m) { // render menu draw menu 
	
	int mH = m->mH;
	int miX = 175;
	int miY = (m->menuVector.size() * mH) + m->menuVector.size();

	m->menuSX = miX;
	m->menuSY = miY - 2; // FIX-Crash when on border

	int posX = (m->menuX > (m->width - m->menuSX)) ? (m->width - m->menuSX) : m->menuX;
	int posY = (m->menuY > (m->height - m->menuSY)) ? (m->height - m->menuSY) : m->menuY;


	m->actmenuX = posX;
	m->actmenuY = posY;

	gaussian_blur((uint32_t*)m->scrdata, miX-4, miY-4, 4.0f, m->width, m->height, posX+2, posY+2);
	dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 0.8f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX+1, posY+1, miX-2, miY-2, 0xFFFFFF, 0.1f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 1.0f);

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	int selected = (mp.y-(posY+2))/mH;
	if (selected < m->menuVector.size()) {
		if (IfInMenu(mp, m)) {
		//if (mp.x > m->actmenuX && mp.y > m->actmenuY && mp.x < (m->actmenuX + m->menuSX) && mp.y < (m->actmenuY + m->menuSY)) {
			int hoverLocX = posX + 4;
			int hoverLocY = posY + 4 + ((mH)*selected);
			int hoverSizeX = miX - 8;
			int hoverSizeY = mH - 3;
			// This is for hovering over your favorite menu button
			dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFF8080, 0.2f);   // FILL
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, hoverLocX, hoverLocY, hoverSizeX, hoverSizeY, 0xFFFFFF, 0.2f);         // white
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, hoverLocX - 1, hoverLocY - 1, hoverSizeX + 2, hoverSizeY + 2, 0x000000, 1.0f); // black BORDER!
		}
	}
	

	SwitchFont(m->Verdana);
	for (int i = 0; i < m->menuVector.size(); i++) {
		 
		std::string mystr = m->menuVector[i].name;
		if (mystr.length() >= 3 && mystr.substr(mystr.length() - 3) == "{s}") {
			mystr = mystr.substr(0, mystr.length() - 3);
		}
		DrawMenuIcon(m, posX + 10, (mH* i) + posY + 10, m->menuVector[i].atlasX, m->menuVector[i].atlasY);
												// changed when adding icon
		PlaceString(m, 12, mystr.c_str(), posX + 27, (mH * i) + posY + 7, 0xF0F0F0, m->scrdata);
		if (i == m->menuVector.size()-1) continue;

		if (strstr(m->menuVector[i].name.c_str(), "{s}")) {
			dDrawFilledRectangle(m->scrdata, m->width, m->height, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.2f);
		}
		else {
			dDrawFilledRectangle(m->scrdata, m->width, m->height, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.05f);
		}
	}
}

void RenderBK(GlobalParams* m, const POINT& p) {
	for(int y=0; y<m->height; y++){
		for (int x = 0; x < m->width; x++) {
			// Don't use anymore due to transparency
				//if (x > (m->CoordRight-(m->mscaler/2)) || x < (m->CoordLeft+(m->mscaler / 2)) || y < (m->CoordTop+(m->mscaler / 2)) || y > (m->CoordBottom-(m->mscaler / 2))) {
					uint32_t bkc = 0x111111;
					// bkc
					if (y > m->toolheight || !(CanRenderToolbarMacro)) {

						if (((x / 9) + (y / 9)) % 2 == 0) {
							bkc = 0x121212;
						}
						else {
							bkc = 0x0C0C0C;
						}

					}


					*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = bkc;
				//}
			}
		}
	
}

void drawCircle(int x, int y, int radius, uint32_t* imageBuffer, int imageWidth) {
	// Ensure non-negative radius
	if (radius < 0) {
		return;
	}

	for (int i = 0; i <= 360; ++i) {
		double radians = i * 3.141592 / 180.0;

		int circleX = static_cast<int>(x + radius * std::cos(radians));
		int circleY = static_cast<int>(y + radius * std::sin(radians));

		// Check if the calculated coordinates are within the image boundaries
		if (circleX >= 0 && circleX < imageWidth && circleY >= 0) {
			// Assuming the image is a linear buffer with each pixel represented by a uint32_t
			imageBuffer[circleY * imageWidth + circleX] = 0xFFFFFFFF; // Set pixel color to white
		}
	}
}

void RenderSlider(GlobalParams* m, int offsetX, int offsetY, int sizex, float position, float highlightOpacity) {
	// draw slider
	
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, offsetX+ 1, offsetY +1, sizex, 5, 0xFFFFFF, highlightOpacity); // actual slider
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, offsetX+ 0, offsetY, sizex+2, 7, 0x000000, 0.9f); // outline slider
	int pos = ((sizex) * position) + offsetX-2;
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, pos+1, offsetY -4, 4, 15, 0xFFFFFF, highlightOpacity); // actual  "knob"
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, pos, offsetY-5, 6, 17, 0x000000, 0.9f); // outline "knob"
}


void DrawDrawModeMenu(GlobalParams* m){
	//if (m->width < 620) { return; }
	SwitchFont(m->SegoeUI);
	m->drawMenuOffsetX = 440; // CHANGED WHEN ADDING SEPERATORS  // -------changed when added copy-----
	m->drawMenuOffsetY = 0;

	int sizeCx = m->width-m->drawMenuOffsetX -80; // offset
	if (sizeCx < 481) { // changed when added copy +31 // changed when adding hard/soft
		m->drawMenuOffsetX = 0;
		m->drawMenuOffsetY = m->height - 40;
		sizeCx = m->width;
		
		boxBlur(GetMemoryLocation(m->scrdata, 0, m->height - 40, m->width, m->height), m->width, 40, 15, 2);
	}


	// draw outline
	dDrawRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX, m->drawMenuOffsetY, sizeCx, 42, 0xFFFFFF, 0.3f);
	// draw main
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX, m->drawMenuOffsetY, sizeCx, 42, 0x000000, 0.5f);

	// draw seperator
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX+75, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 261, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);

	// draw text
	PlaceString(m, 14, "Color:", m->drawMenuOffsetX +11, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);
	PlaceString(m, 14, "Size:", m->drawMenuOffsetX +81, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);
	PlaceString(m, 14, "Opacity:", m->drawMenuOffsetX +270, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);

	char str[256];
	sprintf(str, "%.1fpx", m->drawSize);
	PlaceString(m, 14, str, m->drawMenuOffsetX + 217, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);

	char str2[256];
	sprintf(str2, "%.2fpx", m->a_opacity);
	PlaceString(m, 14, str2, m->drawMenuOffsetX + 410, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);


	// draw color square
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, m->a_drawColor, true); // actual color
	dDrawRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, 0xFFFFFF, 0.3f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX +50, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)
	
	// draw hard/soft area
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 461, m->drawMenuOffsetY + 14, 18, 18, 0x808080, 1.0f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 460, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)

	std::string let = "H";
	if (m->a_softmode) {
		let = "S";
	}

	PlaceString(m, 14, let.c_str(), m->drawMenuOffsetX + 466, m->drawMenuOffsetY + 11, 0x808080, m->scrdata);


	float sizeLeveler = sqrt(m->drawSize-1) / 10.0f; // I used ALGEBRA!
	if (sizeLeveler > 1.0f) { sizeLeveler = 1.0f; }
	if (sizeLeveler < 0.0f) { sizeLeveler = 0.0f; }

	float opacitylever = m->a_opacity;

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	// Hover
	//------------------------------------------------------------------------------------------------------------------------------
	int slider1begin = m->drawMenuOffsetX + m->slider1begin;
	int slider1end = m->drawMenuOffsetX + m->slider1end;

	int slider2begin = m->drawMenuOffsetX + m->slider2begin;
	int slider2end = m->drawMenuOffsetX + m->slider2end;

	int sliderYb = m->drawMenuOffsetY;
	int sliderYe = m->drawMenuOffsetY + 40;

	float highlightOpacity1 = 0.3f;
	float highlightOpacity2 = 0.3f;

	if (CheckIfMouseInSlider1(mp, m, slider1begin, slider1end, sliderYb, sliderYe) || m->slider1mousedown) {
		highlightOpacity1 = 0.45f;
	}

	if (CheckIfMouseInSlider2(mp, m, slider2begin, slider2end, sliderYb, sliderYe) || m->slider2mousedown) {
		highlightOpacity2 = 0.45f;
	}
	//------------------------------------------------------------------------------------------------------------------------------


	RenderSlider(m, m->drawMenuOffsetX +m->slider1begin, m->drawMenuOffsetY + 19, m->slider1end-m->slider1begin, sizeLeveler, highlightOpacity1);
	RenderSlider(m, m->drawMenuOffsetX +m->slider2begin, m->drawMenuOffsetY + 19, m->slider2end-m->slider2begin, opacitylever, highlightOpacity2);

	//FT_Done_Face(face);
	//FT_Done_FreeType(ft);


}

BITMAPINFO bmi;
void UpdateBuffer(GlobalParams* m) {
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = m->width;
	bmi.bmiHeader.biHeight = -(int64_t)m->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// tell our lovely win32 api to update the window for us <3
	SetDIBitsToDevice(m->hdc, 0, 0, m->width, m->height, 0, 0, 0, m->height, m->scrdata, &bmi, DIB_RGB_COLORS);
}

void Tint(GlobalParams* m) {
	for(int y=0; y<m->height; y++) {
		for (int x = 0; x< m->width; x++) {
					if (((x) + (y)) % 2 == 0) {
						*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = 0x000000;
					}
				}
		}
}

void RenderCropButton(GlobalParams* m, int px, int py, bool invertX, bool invertY, bool selected) {
	if (px >= m->width - 16) { return; }
	if (py >= m->height - 16) { return; }
	if (px < 1) { return; }
	if (py < 1) { return; }

	uint32_t distLeft, distRight, distTop, distBottom;
	GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);

	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			
			uint32_t* from = GetMemoryLocation(m->cropImageData, x, y, 16, 16);
			
			int xp = (-x + (2 * x * (1 - invertX))) + (invertX * 15); // math without logic
			int yp = (-y + (2 * y * (1 - invertY))) + (invertY * 15);
			int toLocX = xp + px;
			int toLocY = yp + py;

			// check within bounds

			uint32_t* to = GetMemoryLocation(m->scrdata, toLocX,toLocY, m->width, m->height);
			if (*from != 0xFFFF00FF) {
				if (selected) {
					*to = 0xFFFFFF;
				}
				else {
					*to = *from;
				}
			}
		}
	}
}

void DrawFrame(GlobalParams* m, uint32_t framecolor, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom) {

	// sorry for the mess
	int wFrame = right - left;
	int hFrame = bottom - top;
	for (int y = 0; y < hFrame; y++) {
		if (((top + y) > 0 && (top + y) <= m->height) && ((left) > 0 && (left) <= m->width)) {
			*GetMemoryLocation(m->scrdata, left, top + y, m->width, m->height) = framecolor;
		}
	}
	for (int y = 0; y < hFrame; y++) {
		if (((top + y) > 0 && (top + y) <= m->height) && ((right) > 0 && (right) <= m->width)) {
			*GetMemoryLocation(m->scrdata, right, top + y, m->width, m->height) = framecolor;
		}
	}
	for (int x = 0; x < wFrame; x++) {
		if (((left + x) > 0 && (left + x) <= m->width) && ((top) > 0 && (top) <= m->height)) {
			*GetMemoryLocation(m->scrdata, left + x, top, m->width, m->height) = framecolor;
		}
	}
	for (int x = 0; x < wFrame; x++) {
		if (((left + x) > 0 && (left + x) <= m->width) && ((bottom) > 0 && (bottom) <= m->height)) {
			*GetMemoryLocation(m->scrdata, left + x, bottom, m->width, m->height) = framecolor;
		}
	}
}

void RenderCropGUI(GlobalParams* m) {
	// crop UI

	for (int y = 0; y < m->height; y++) {
		for (int x = 0; x < m->width; x++) {
			// tint screen
			uint32_t* data = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
			*data = lerp(0x000000, *data, 0.3f);
			
		}
	}

	SwitchFont(m->OCRAExt);
	PlaceString(m, 14, "Move handles with mouse then right click to confirm changes", 10, 10, 0xFFFFFF, m->scrdata);

	

	uint32_t distLeft, distRight, distTop, distBottom;
	GetCropCoordinates(m, &distLeft, &distRight, &distTop, &distBottom);




	// render crop thingy
	RenderCropButton(m, distLeft, distTop, false, false, m->CropHandleSelectTL);
	RenderCropButton(m, distRight-16, distTop, true, false, m->CropHandleSelectTR);
	RenderCropButton(m, distLeft, distBottom-16, false, true, m->CropHandleSelectBL);
	RenderCropButton(m, distRight-16, distBottom-16, true, true, m->CropHandleSelectBR);

	int widthOfImage = m->imgwidth * m->mscaler;
	int heightOfImage = m->imgheight * m->mscaler;

	DrawFrame(m, 0xFFFFFF, distLeft, distRight, distTop, distBottom);

	//dDrawRectangle(m->scrdata, m->width, m->height, m->CoordLeft, m->CoordTop, widthOfImage, heightOfImage, 0xFFFFFF, 0.5f);


}

void RedrawSurface(GlobalParams* m, bool onlyImage, bool doesManualClip) {
	if (!doesManualClip) {
		SelectClipRgn(m->hdc, NULL);
	}
	if (m->sleepmode && (!m->isImagePreview)) {
		return;
	}
	if (m->width < 20) { return; }

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);

	// set the coordinates for the image
	if ((!onlyImage)) {
		// We aren't going to reset the coordinates due to the coordinates already being reset by 
		// the function calling redrawsurface. when passing in onlyimage we assume this is being done
		ResetCoordinates(m);
	}


	void* drawingbuffer = m->imgdata;
	if (m->isImagePreview) {
		drawingbuffer = m->imagepreview;
	}

	if (drawingbuffer) {
		if ((m->smoothing && ((!m->mouseDown) || m->drawmode)) && !m->isInCropMode) {
			PlaceImageBI(m, drawingbuffer, true, p);
		}
		else {
			PlaceImageNN(m, drawingbuffer, true, p);
		}
	}
	else if (m->imgwidth > 1) {
		MessageBox(m->hwnd, "Failed to obtain drawing buffer", "Error", MB_OK | MB_ICONERROR);
		exit(0);
	} else {
		RenderBK(m, p);
	} 


	// draw 1px frame
	if (!m->isInCropMode) {
		DrawFrame(m, 0x000000, m->CoordLeft, m->CoordRight, m->CoordTop, m->CoordBottom);
	}

	//dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->CoordLeft, m->CoordTop, wFrame, hFrame, 0x808080, 1.0f);


	// debug circles
	//drawCircle(m->CoordLeft, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);
	//drawCircle(m->CoordRight, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);

	if ((CanRenderToolbarMacro) && (!onlyImage)) {
		RenderToolbar(m);

	}
	if ((CanRenderToolbarMacro)) {
		RenderToolbarShadow(m);
	}

	bool shouldIDrawTheDrawingToolbar = !onlyImage;
	if (m->drawMenuOffsetY > m->toolheight) {
		shouldIDrawTheDrawingToolbar = true;
	}
	if (m->drawmode && (shouldIDrawTheDrawingToolbar) && (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight) || m->drawMenuOffsetY > 1)) {
		DrawDrawModeMenu(m);
	}



	if (m->drawmode) {
		SwitchFont(m->SegoeUI);
		PlaceStringShadow(m, 12, "Draw Mode", 12, m->toolheight + 25, 0xFFFFFF, m->scrdata, 2);
		PlaceStringShadow(m, 10, "LEFT: Draw", 12, m->toolheight + 40, 0xC0C0C0, m->scrdata, 1);
		PlaceStringShadow(m, 10, "MIDDLE: Pan", 12, m->toolheight + 50, 0xC0C0C0, m->scrdata, 1);
		PlaceStringShadow(m, 10, "CTRL/RIGHT CLICK: Erase", 12, m->toolheight + 60, 0xC0C0C0, m->scrdata, 1);
		PlaceStringShadow(m, 10, "CTRL+SHIFT: Transparent", 12, m->toolheight + 70, 0xC0C0C0, m->scrdata, 1);
		PlaceStringShadow(m, 10, "SHIFT+Z: Eyedropper", 12, m->toolheight + 80, 0xC0C0C0, m->scrdata, 1);
	}



	if (m->isInCropMode) {
		RenderCropGUI(m);
	}


	if (m->isMenuState) {
		DrawMenu(m);
	}
	// draw test circle
	/*
	
	if (m->isAnnotationCircleShown) {
		CircleGenerator(m->drawSize * m->mscaler, p.x, p.y, 12, 4, (uint32_t*)m->scrdata, 0x000000, 1.0f, m->width, m->height);
		CircleGenerator(m->drawSize * m->mscaler-2, p.x, p.y, 5, 4, (uint32_t*)m->scrdata, 0xFFFFFF, 0.5f, m->width, m->height);
		
	}
	*/
	
	//InitFont(m->hwnd, "segoeui.TTF", 14); // why did I even put this here? it just causes a memory leak and does nothing
	
	 // FOR DEBUG
	/*
	
	uint32_t randc = ((uint32_t)rand() << 16) | (uint32_t)rand();;
		for (int y = 0; y < m->height; y++) {
			for (int x = 0; x < m->width; x++) {
				uint32_t* memloc = GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
				*memloc = lerp(*memloc, randc, 0.5f);
			}
		}
	*/
	
	
	if (m->tint || m->deletingtemporaryfiles) {
		Tint(m);
	}

	if (m->loading) {
		SwitchFont(m->SegoeUI);
		PlaceStringShadow(m, 20, "Loading", 10, m->toolheight + 10, 0xFFFFFF, m->scrdata, 2, 2);
	}

	if (m->deletingtemporaryfiles) {
		SwitchFont(m->OCRAExt);
		PlaceString(m, 30, "-- Maintenance --", 33, 50, 0xFFFFFF, m->scrdata);
		PlaceString(m, 50, "Deleting temporary files", 33, 100, 0xFFFFFF, m->scrdata);
	}


	// debug mode

	SwitchFont(m->SegoeUI);
	if (m->debugmode) {
		char debug[256];
		sprintf(debug, "MS: %f: RES: %f: Undo Queue: %d:Undo Step: %d: Elapsed debug time: %f", m->ms_time, m->a_resolution, m->ProcessOfMakingUndoStep, m->undoStep, m->etime);
		PlaceString(m, 16, debug, 12, m->toolheight + 8, 0x808080, m->scrdata);

	}


	// update buffer
	UpdateBuffer(m);

	// Update window title
	
	
	std::string aststring = "";
	if (m->shouldSaveShutdown) {
		aststring = "*";
	}

	char str[256];
	if (!m->fpath.empty()) {
		sprintf(str, "View Image | %s%s | %d\%%", m->fpath.c_str(), aststring.c_str(), (int)(m->mscaler * 100.0f));
	}
	else {
		sprintf(str, "View Image");
	}
	SetWindowText(m->hwnd, str);

}