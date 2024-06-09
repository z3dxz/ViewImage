
#include "headers/rendering.hpp"
#include <vector>
#include <thread>
#include <time.h>
#include <shlwapi.h>
#include <wincodec.h>

#pragma comment(lib,"freetype.lib")

// Freetype Globals
FT_Library ft;
FT_Face face;


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

bool isalreadyopened = false;
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
			MessageBox(m->hwnd, "Failed to load a font. This dialog will not appear again.", "Error", MB_OK | MB_ICONERROR);
			isalreadyopened = true;
		}
		m->fontinit = false;
		return false;
	}

	FT_Set_Pixel_Sizes(face, 0, size);
	m->fontinit = true;
	return true;
}

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

int PlaceString(GlobalParams* m, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem) {
	if (!m->fontinit) {
		return 0;
	}
	if (m->width < 100) return 0;
	const char* text = inputstr;

	FT_GlyphSlot g = face->glyph;
	int penX = 0;

	for (const char* p = text; *p; ++p) {
		if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
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
				int pixelIndex = (y)*m->width + (penX + x);
				unsigned char pixelValue = bitmap->buffer[y * bitmap->width + x];
				uint32_t ptx = locX + penX + x;
				uint32_t pty = locY + (face->size->metrics.ascender >> 6) - (bitmap->rows - y) + maxDescender;

				uint32_t* memoryPath = GetMemoryLocation(mem, ptx, pty, m->width, m->height);
				uint32_t existingColor = *memoryPath;

				*GetMemoryLocation(mem, ptx, pty, m->width, m->height) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
			}
		}

		// Calculate actual character width
		int characterWidth = (g->metrics.horiAdvance >> 6) - (g->metrics.horiBearingX >> 6);
		penX += characterWidth;
	}

	return 1;
}

int PlaceString_old_legacy(GlobalParams* m, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem) {

	if (m->width < 100) return 0;
	const char* text = inputstr;

	FT_GlyphSlot g = face->glyph;
	int penX = 0;

	for (const char* p = text; *p; ++p) {
		if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
			continue;

		FT_Bitmap* bitmap = &g->bitmap;


		int yOffset = (g->metrics.horiBearingY - g->bitmap_top) >> 6;

		int maxGlyphHeight = 0;
		int maxDescender = 0;
		if (g->bitmap.rows > maxGlyphHeight)
			maxGlyphHeight = g->bitmap.rows;

		int descender = (g->metrics.height >> 6) - yOffset;
		if (descender > maxDescender)
			maxDescender = descender;

		for (int y = 0; y < bitmap->rows; ++y) {
			for (int x = 0; x < bitmap->width; ++x) {
				int pixelIndex = (y)*m->width + (penX + x);
				unsigned char pixelValue = bitmap->buffer[y * bitmap->width + x];
				uint32_t ptx = locX + penX + x;
				uint32_t pty = locY + (face->size->metrics.ascender >> 6) - (bitmap->rows - y) + maxDescender;

				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, ptx, pty, m->width, m->height);
				uint32_t existingColor = *memoryPath;
				
				*GetMemoryLocation(mem, ptx, pty, m->width, m->height) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
			}
		}

		penX += g->advance.x >> 6;
	}


	return true;

}


void PlaceNewFont(GlobalParams* m, std::string text, int x, int y, std::string FontA, int size, uint32_t color) {
	if (m->fontinit) {
		FT_Done_Face(face);
		FT_Done_FreeType(ft);
	}
	InitFont(m, FontA, size);
	PlaceString(m, text.c_str(), x, y, color, m->scrdata);

}


void dDrawFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
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

void dDrawRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
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
}

void dDrawRoundedFilledRectangle(void* mem, int kwidth, int kheight, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth, kheight);
			//if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1) && !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
					if (opacity < 0.99f) {
						*ma = lerp(*ma, color, opacity);
					}
					else {
						*ma = color;
					}
				}
			//}
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

bool paintBG = true;

// MULTITHREADING!!!!!
// https://www.youtube.com/watch?v=46ddlUImiQA
//*********************************************

void PlaceImageNN(GlobalParams* m, void* memory, bool invert) {
	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth = m->imgwidth;
	const int32_t imgheight = m->imgheight;
	const int margin = 2;

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {
			int32_t offX = (((int32_t)m->width - (int32_t)(imgwidth * m->mscaler)) / 2) + m->iLocX;
			int32_t offY = (((int32_t)m->height - (int32_t)(imgheight * m->mscaler)) / 2) + m->iLocY;

			int32_t ptx = (x - offX) * inv_mscaler;
			int32_t pty = (y - offY) * inv_mscaler;

			if (ptx < imgwidth && pty < imgheight && ptx >= 0 && pty >= 0 &&
				x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) {
				uint32_t c = *GetMemoryLocation(memory, ptx, pty, imgwidth, imgheight);

				int alpha = (c >> 24) & 255;
				uint32_t doColor = c;
				if (alpha != 255) {
					uint32_t existing = *GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
					doColor = lerp(existing, c, static_cast<float>(alpha) / 255.0f);
				}
				*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
			}
		}
		});
}


bool followsPattern(int number) {
	double logBase2 = log2(number / 100.0);
	return logBase2 == floor(logBase2);
}

void PlaceImageBI(GlobalParams* m, void* memory, bool invert) {
	const int margin = m->fullscreen ? 0 : 2;
	const float inv_mscaler = 1.0f / m->mscaler;
	const int32_t imgwidth_minus_1 = m->imgwidth - 1;
	const int32_t imgheight_minus_1 = m->imgheight - 1;
	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), [&](uint32_t y) {
		for (uint32_t x : m->ith) {
			int32_t offX = (((int32_t)m->width - (int32_t)(m->imgwidth * m->mscaler)) / 2) + m->iLocX;
			int32_t offY = (((int32_t)m->height - (int32_t)(m->imgheight * m->mscaler)) / 2) + m->iLocY;

			float ptx = (x - offX) * inv_mscaler - 0.5f;
			float pty = (y - offY) * inv_mscaler - 0.5f;

			if (followsPattern(m->mscaler * 100)) {
				ptx = (x - offX) * inv_mscaler;
				pty = (y - offY) * inv_mscaler;
			}

			// 

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

				// Bilinear interpolation
				uint32_t doColor = lerp(lerp(c00, c01, ptx_frac), lerp(c10, c11, ptx_frac), pty_frac);
				int alpha = (doColor >> 24) & 255;
				if (alpha != 255) {
					uint32_t existing = *GetMemoryLocation(m->scrdata, x, y, m->width, m->height);
					doColor = lerp(existing, doColor, static_cast<float>(alpha) / 255.0f);
				}
				*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = doColor;
			}
		}
		});
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

void RenderToolbar(GlobalParams* m) {


		// Render the toolbar


		//BLUR FOR TOOLBAR
	
		if (m->CoordTop <= m->toolheight) {
			gaussian_blur_toolbar(m, (uint32_t*)m->scrdata);
		}
		else {

			for (uint32_t y = 0; y < m->toolheight; y++) {
				for (uint32_t x = 0; x < m->width; x++) {
					*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = 0x111111;
				}
			}
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
		
		// drop shadow for the toolbar (not buttons)
		for (uint32_t y = 0; y < 20; y++) {
			for (uint32_t x = 0; x < m->width; x++) {
				uint32_t color = 0x000000;

				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y + m->toolheight, m->width, m->height);
				*memoryPath = lerp(*memoryPath, color, (1.0f - ((float)y / 20.0f)) * 0.3f); // transparency
			}
		}
		// BUTTONS actually


		if (!m->toolbarData) return;
		if (!m->toolbarData_shadow) return;

		// render toolbar buttons shadow

		int mybutton = m->maxButtons;
		if (m->imgwidth < 1) mybutton = 1;

		RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x606060, 0.7f, 0, 0.7f);
		RenderToolbarButtonShadow(m, m->toolbarData_shadow, mybutton, 0x000000, 0.7f, 0, 0.7f);
		RenderMainToolbarButtons(m, m->toolbarData, mybutton, 0xFFFFFF, 0.7f, 0xFFE0E0, 1.0f);


		// tooltips AND OUTLINE FOR THE BUTTONS (basically stuff when its selected)

		//-- The border when selecting annotate
		if (m->drawmode) {
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (7 * GetButtonInterval(m) + 2), 4, GetButtonInterval(m), m->toolheight - 8, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (7 * GetButtonInterval(m) + 2) - 1, 3, GetButtonInterval(m) + 2, m->toolheight - 6, 0x000000, 1.0f);
		}

		if (m->selectedbutton >= 0 && m->selectedbutton < mybutton) {
			// rounded corners: split hover thing into three things

			//-- The fill when selecting the buttons
				dDrawFilledRectangle(m->scrdata, m->width, m->height, (m->selectedbutton * GetButtonInterval(m) + 2) + 1, 4, GetButtonInterval(m) - 2, 1, 0xFF8080, 0.2f);
				dDrawFilledRectangle(m->scrdata, m->width, m->height, (m->selectedbutton * GetButtonInterval(m) + 2), 5, GetButtonInterval(m), m->toolheight - 10, 0xFF8080, 0.2f);
				dDrawFilledRectangle(m->scrdata, m->width, m->height, (m->selectedbutton * GetButtonInterval(m) + 2) + 1, (m->toolheight - 5), GetButtonInterval(m) - 2, 1, 0xFF8080, 0.2f);
	
			//-- The outline wb border when selecting the buttons
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (m->selectedbutton*GetButtonInterval(m)+2), 4, GetButtonInterval(m), m->toolheight - 8, 0xFFFFFF, 0.2f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, (m->selectedbutton * GetButtonInterval(m) + 2)-1, 3, GetButtonInterval(m)+2, m->toolheight - 6, 0x000000, 1.0f);

			

			std::string txt = "Error";
			if (m->selectedbutton < m->maxButtons) {
				txt = strtable[m->selectedbutton];
			}

			int loc = 1 + (m->selectedbutton * GetButtonInterval(m) + 2);
			dDrawFilledRectangle(m->scrdata, m->width, m->height, loc, m->toolheight + 5, (txt.length() * 8) + 10, 18, 0x000000, 0.8f);
			dDrawRoundedRectangle(m->scrdata, m->width, m->height, loc - 1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0x808080, 0.4f);
			PlaceNewFont(m, txt.c_str(), loc + 4, m->toolheight + 2, "segoeui.TTF", 14, 0xFFFFFF);


			
		}

		// fullscreen icon
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

			//-- Fullscreen icon tooltip
			if (m->fullscreen) {
				PlaceNewFont(m, "Exit Fullscreen (F11)", m->width - 120, 48, "tahoma.ttf", 13, 0xFFFFFF);
			}
			else {
				PlaceNewFont(m, "Fullscreen (F11)", m->width - 100, 48, "tahoma.ttf", 13, 0xFFFFFF);
			}
		}

		for (int y = 0; y < 11; y++) {
			for (int x = 0; x < 12; x++) {
				uint32_t* oLoc = GetMemoryLocation(m->scrdata, m->width + x - 30, y + 18, m->width, m->height);
				float t = 0.5f;
				if (nearf) {
					t = 1.0f;
				}
				float transparency = ((float)((*GetMemoryLocation(m->fullscreenIconData, x, y, 12, 11))&0xFF) / 255.0f)*t;
				*oLoc = lerp(*oLoc, 0xFFFFFF, transparency);
			}
		}

		// version
		PlaceNewFont(m, REAL_BIG_VERSION_BOOLEAN, m->width - 70, 16, "OCRAEXT.TTF", 13, 0x808080);

		
		
}

void ResetCoordinates(GlobalParams* m) {

	if (m->imgwidth > 0) {

		m->CoordLeft = (m->width / 2) - (-m->iLocX) - (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordTop = (m->height / 2) - (-m->iLocY) - (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

		m->CoordRight = (m->width / 2) - (-m->iLocX) + (int)((((float)m->imgwidth / 2.0f)) * m->mscaler);
		m->CoordBottom = (m->height / 2) - (-m->iLocY) + (int)((((float)m->imgheight / 2.0f)) * m->mscaler);

	}
}

void DrawMenu(GlobalParams* m) {
	int mH = m->mH;

	int miX = 150;
	int miY = (m->menuVector.size() * mH) + m->menuVector.size();

	m->menuSX = miX;
	m->menuSY = miY - 2; // FIX-Crash when on border

	int posX = m->menuX;
	int posY = m->menuY;

	gaussian_blur((uint32_t*)m->scrdata, miX-4, miY-4, 4.0f, m->width, m->height, posX+2, posY+2);
	dDrawFilledRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 0.6f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX+1, posY+1, miX-2, miY-2, 0xFFFFFF, 0.1f);
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, posX, posY, miX, miY, 0x000000, 1.0f);

	POINT mp;
	GetCursorPos(&mp);
	ScreenToClient(m->hwnd, &mp);

	int selected = (mp.y-(posY+2))/mH;

	if (mp.x > m->menuX && mp.y > m->menuY && mp.x < (m->menuX + m->menuSX) && mp.y < (m->menuY + m->menuSY)) {
		dDrawRoundedFilledRectangle(m->scrdata, m->width, m->height, posX + 4, posY + 4 + ((mH)*selected), miX - 8, mH - 3, 0xFF8080, 0.1f);
	}

	for (int i = 0; i < m->menuVector.size(); i++) {
		 
		std::string mystr = m->menuVector[i].first;
		if (mystr.length() >= 3 && mystr.substr(mystr.length() - 3) == "{s}") {
			mystr = mystr.substr(0, mystr.length() - 3);
		}

		PlaceNewFont(m, mystr, posX + 10, (mH * i)+posY+7, "Verdana.TTF", 12, 0xF0F0F0);
		if (i == m->menuVector.size()-1) continue;

		if (strstr(m->menuVector[i].first.c_str(), "{s}")) {
			dDrawFilledRectangle(m->scrdata, m->width, m->height, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.2f);
		}
		else {
			dDrawFilledRectangle(m->scrdata, m->width, m->height, posX + 8, posY + mH + (mH * i) + 2, miX - 16, 1, 0xFFFFFF, 0.05f);
		}
	}
}

void RenderBK(GlobalParams* m) {
	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(), // MULTITHREADING!!
		[&](uint32_t y) {
			std::for_each(std::execution::par, m->ith.begin(), m->ith.end(),
			[&](uint32_t x) {
					uint32_t bkc{};
					// bkc
					if (((x / 9) + (y / 9)) % 2 == 0) {
						bkc = 0x121212;
					}
					else {
						bkc = 0x0C0C0C;
					}

					*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = bkc;
				});
		});
	
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

	m->drawMenuOffsetX = 424; // changed when added copy
	m->drawMenuOffsetY = 0;

	int sizeCx = m->width-m->drawMenuOffsetX -80; // offset
	if (sizeCx < 481) { // changed when added copy +31 // changed when adding hard/soft
		m->drawMenuOffsetX = 0;
		m->drawMenuOffsetY = m->height - 40;
		sizeCx = m->width;

		boxBlur(GetMemoryLocation(m->scrdata, 0, m->height - 40, m->width, m->height), m->width, 40, 15);
	}

	

	// draw outline
	dDrawRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX, m->drawMenuOffsetY, sizeCx, 42, 0xFFFFFF, 0.3f);
	// draw main
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX, m->drawMenuOffsetY, sizeCx, 42, 0x000000, 0.5f);

	// draw seperator
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX+75, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 261, m->drawMenuOffsetY + 15, 1, 17, 0xFFFFFF, 0.3f);

	// draw text
	PlaceNewFont(m, "Color:", m->drawMenuOffsetX +11, m->drawMenuOffsetY + 10, "segoeui.TTF", 14, 0xFFFFFF);
	PlaceString(m, "Size:", m->drawMenuOffsetX +81, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);
	PlaceString(m, "Opacity:", m->drawMenuOffsetX +270, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);

	char str[256];
	sprintf(str, "%.1fpx", m->drawSize);
	PlaceString(m, str, m->drawMenuOffsetX + 217, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);

	char str2[256];
	sprintf(str2, "%.2fpx", m->a_opacity);
	PlaceString(m, str2, m->drawMenuOffsetX + 410, m->drawMenuOffsetY + 10, 0xFFFFFF, m->scrdata);


	// draw color square
	dDrawFilledRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, m->a_drawColor, true); // actual color
	dDrawRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 51, m->drawMenuOffsetY + 14, 18, 18, 0xFFFFFF, 0.3f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX +50, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)
	
	// draw hard/soft area
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 461, m->drawMenuOffsetY + 14, 18, 18, 0xFFFFFF, 0.3f); // outline (white)
	dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->drawMenuOffsetX + 460, m->drawMenuOffsetY + 13, 20, 20, 0x000000, 0.9f); // outline (black)

	std::string let = "H";
	if (m->a_softmode) {
		let = "S";
	}

	PlaceString(m, let.c_str(), m->drawMenuOffsetX + 466, m->drawMenuOffsetY + 11, 0x555555, m->scrdata);


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

	if ((mp.x > slider1begin && mp.x < slider1end) && (mp.y > sliderYb && mp.y < sliderYe) || m->slider1mousedown) {
		highlightOpacity1 = 0.45f;
	}

	if ((mp.x > slider2begin && mp.x < slider2end) && (mp.y > sliderYb && mp.y < sliderYe) || m->slider2mousedown) {
		highlightOpacity2 = 0.45f;
	}
	//------------------------------------------------------------------------------------------------------------------------------


	RenderSlider(m, m->drawMenuOffsetX +m->slider1begin, m->drawMenuOffsetY + 19, m->slider1end-m->slider1begin, sizeLeveler, highlightOpacity1);
	RenderSlider(m, m->drawMenuOffsetX +m->slider2begin, m->drawMenuOffsetY + 19, m->slider2end-m->slider2begin, opacitylever, highlightOpacity2);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	InitFont(m, "segoeui.TTF", 14);


}

void RedrawSurface(GlobalParams* m) {
	if (m->sleepmode) {
		return;
	}

	
	if (m->width < 20) { return; }
	uint32_t color = 0x101010;


	if (GetKeyState('L') & 0x8000 && GetKeyState('P') & 0x8000 && GetKeyState('M') & 0x8000 && GetKeyState(VK_RCONTROL) && 0x8000) {
		// DEBUG: Turn off background placement
		paintBG = !paintBG;
	}

	// render the background
	if (paintBG) {
		RenderBK(m);
	}

	// render the image
	//if (!m->loading) {
	
	void* drawingbuffer = m->imgdata;
	if (m->smoothing && ((!m->mouseDown) || m->drawmode)) {
		PlaceImageBI(m, drawingbuffer, true);
	}
	else {
		PlaceImageNN(m, drawingbuffer, true);
	}
	
	if (m->shouldSaveShutdown) {
		//PlaceImageBI(m, m->img, false);
	}

	// set the coordinates for the image
	ResetCoordinates(m);

	// draw 1px frame
	uint32_t framecolor = 0x000000;
	{ // sorry for the mess
		int wFrame = m->CoordRight - m->CoordLeft;
		int hFrame = m->CoordBottom - m->CoordTop;
		for (int y = 0; y < hFrame; y++) {
			if (((m->CoordTop + y) > 0 && (m->CoordTop + y) <= m->height) && ((m->CoordLeft) > 0 && (m->CoordLeft) <= m->width)) {
				*GetMemoryLocation(m->scrdata, m->CoordLeft, m->CoordTop + y, m->width, m->height) = framecolor;
			}
		}
		for (int y = 0; y < hFrame; y++) {
			if (((m->CoordTop + y) > 0 && (m->CoordTop + y) <= m->height) && ((m->CoordRight) > 0 && (m->CoordRight) <= m->width)) {
				*GetMemoryLocation(m->scrdata, m->CoordRight, m->CoordTop + y, m->width, m->height) = framecolor;
			}
		}
		for (int x = 0; x < wFrame; x++) {
			if (((m->CoordLeft + x) > 0 && (m->CoordLeft + x) <= m->width) && ((m->CoordTop) > 0 && (m->CoordTop) <= m->height)) {
				*GetMemoryLocation(m->scrdata, m->CoordLeft + x, m->CoordTop, m->width, m->height) = framecolor;
			}
		}
		for (int x = 0; x < wFrame; x++) {
			if (((m->CoordLeft + x) > 0 && (m->CoordLeft + x) <= m->width) && ((m->CoordBottom) > 0 && (m->CoordBottom) <= m->height)) {
				*GetMemoryLocation(m->scrdata, m->CoordLeft + x, m->CoordBottom, m->width, m->height) = framecolor;
			}
		}
	}

	

	//dDrawRoundedRectangle(m->scrdata, m->width, m->height, m->CoordLeft, m->CoordTop, wFrame, hFrame, 0x808080, 1.0f);

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);

	// debug circles
	//drawCircle(m->CoordLeft, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);
	//drawCircle(m->CoordRight, m->CoordTop, 16, (uint32_t*)m->scrdata, m->width);

	
	if ((!m->fullscreen && m->height >= 250) || p.y < m->toolheight) {
		RenderToolbar(m);
		
	}
	
	if (m->drawmode && (((!m->fullscreen && m->height >= 250) || p.y < m->toolheight) || m->drawMenuOffsetY>1)) {
		DrawDrawModeMenu(m);
	}

	if (m->isMenuState) {
		DrawMenu(m);
	}

	if (m->loading) {

		PlaceNewFont(m, "Loading", 12, m->toolheight + 12, "segoeui.TTF", 20, 0x000000);
		PlaceString(m, "Loading", 10, m->toolheight + 10, 0xFFFFFF, m->scrdata);

		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		InitFont(m, "segoeui.TTF", 14);
	}

	if (m->drawmode) {

		PlaceNewFont(m, "Draw Mode", 12, m->toolheight + 27, "segoeui.TTF", 12, 0x000000);
		PlaceNewFont(m, "Draw Mode", 12, m->toolheight + 25, "segoeui.TTF", 12, 0xFFFFFF);
		PlaceNewFont(m, "LEFT: Draw", 12, m->toolheight + 40, "segoeui.TTF", 10, 0x808080);
		PlaceNewFont(m, "MIDDLE: Pan", 12, m->toolheight + 50, "segoeui.TTF", 10, 0x808080);
		PlaceNewFont(m, "CTRL/RIGHT CLICK: Erase", 12, m->toolheight + 60, "segoeui.TTF", 10, 0x808080);
		PlaceNewFont(m, "CTRL+SHIFT: Transparent", 12, m->toolheight + 70, "segoeui.TTF", 10, 0x808080);
		PlaceNewFont(m, "SHIFT+Z: Eyedropper", 12, m->toolheight + 80, "segoeui.TTF", 10, 0x808080);

		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		InitFont(m, "segoeui.TTF", 14);
	}


	if (((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) && (m->imgwidth >= 1)) {// TWINNING!

		PlaceNewFont(m, "CTRL + ALT: Keyboard Mode - Press any number to select the corresponding toolbar button", 12, m->toolheight + 13, "segoeui.TTF", 16, 0x000000);
		PlaceNewFont(m, "CTRL + ALT: Keyboard Mode - Press any number to select the corresponding toolbar button", 12, m->toolheight + 11, "segoeui.TTF", 16, 0xFFFFFF);

		FT_Done_Face(face);
		FT_Done_FreeType(ft);
		InitFont(m, "segoeui.TTF", 14);
	}

	if (m->debugmode) {
		char debug[256];
		sprintf(debug, "MS: %f: RES: %f", m->ms_time, m->a_resolution);
		PlaceNewFont(m, debug, 12, m->toolheight + 8, "segoeui.TTF", 16, 0x808080);

	}

	// draw test circle
	/*
	
	if (m->isAnnotationCircleShown) {
		CircleGenerator(m->drawSize * m->mscaler, p.x, p.y, 12, 4, (uint32_t*)m->scrdata, 0x000000, 1.0f, m->width, m->height);
		CircleGenerator(m->drawSize * m->mscaler-2, p.x, p.y, 5, 4, (uint32_t*)m->scrdata, 0xFFFFFF, 0.5f, m->width, m->height);
		
	}
	*/
	
	//InitFont(m->hwnd, "segoeui.TTF", 14); // why did I even put this here? it just causes a memory leak and does nothing


	if (m->pinkTestCenter) {
		for (int y = 0; y < m->height; y++) {
			for (int x = 0; x < m->width; x++) {
				*GetMemoryLocation(m->scrdata, x, y, m->width, m->height) = 0xFF00FF;
			}
		}
	}
	


	// Update window title
	
	
	char str[256];
	if (!m->fpath.empty()) {
		sprintf(str, "View Image | %s | %d\%%", m->fpath.c_str(), (int)(m->mscaler * 100.0f));
	}
	else {

		sprintf(str, "View Image");
	}
	SetWindowText(m->hwnd, str);

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = m->width;
	bmi.bmiHeader.biHeight = -(int64_t)m->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// tell our lovely win32 api to update the window for us <3
	SetDIBitsToDevice(m->hdc, 0, 0, m->width, m->height, 0, 0, 0, m->height, m->scrdata, &bmi, DIB_RGB_COLORS);


}