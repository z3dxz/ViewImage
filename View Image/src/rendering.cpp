
#include "headers/rendering.h"
#include <vector>
#include <thread>

// Freetype Globals
FT_Library ft;
FT_Face face;

bool InitFont(HWND hwnd, const char* font, int size) {

	if (FT_Init_FreeType(&ft)) {
		MessageBox(hwnd, "Failed to initialize FreeType library\n", "Error", MB_OK);
		return false;
	}

	if (FT_New_Face(ft, font, 0, &face)) {
		MessageBox(hwnd, "Failed to load font\n", "Error", MB_OK);
		return false;
	}

	FT_Set_Pixel_Sizes(face, 0, size);

}

const char* strtable[] {
	"Open Image",
	"Save as PNG",
	"Zoom In",
	"Zoom Out",
	"Zoom Auto",
	"Zoom 1:1 (100%)",
	"Rotate",
	"DELETE image",
	"Information"
};

int RenderStringFancy(GlobalParams* m, const char* inputstr, uint32_t locX, uint32_t locY, uint32_t color, void* mem) {


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

				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, ptx, pty, m->width);
				uint32_t existingColor = *memoryPath;

				*GetMemoryLocation(mem, ptx, pty, m->width) = lerp(existingColor, color, ((float)pixelValue / 255.0f));
			}
		}

		penX += g->advance.x >> 6;
	}


	return true;

}

void dDrawFilledRectangle(void* mem, int kwidth, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth);
			if (opacity < 0.99f) {
				*ma = lerp(*ma, color, opacity);
			}
			else {
				*ma = color;
			}
		}
	}
}

void dDrawRectangle(void* mem, int kwidth, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth);
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

void dDrawRoundedRectangle(void* mem, int kwidth, int xloc, int yloc, int width, int height, uint32_t color, float opacity) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t* ma = GetMemoryLocation(mem, xloc + x, yloc + y, kwidth);
			if (x < 1 || x > width - 2 || y < 1 || y > height - 2) {
				if (!(x == 0 && y == 0) && !(x == width - 1 && y == height - 1) && !(x == width - 1 && y == 0) && !(x == 0 && y == height - 1)) {
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

//*********************************************
void RedrawImageOnBitmap(GlobalParams* m) {
	// Clear the bitmap


	uint32_t color = 0x101010;

	bool paintBG = true;

	if (GetKeyState('L') & 0x8000 && GetKeyState('P') & 0x8000 && GetKeyState('M') & 0x8000) {
		paintBG = false;
	}

	// render the image
	// MULTITHREADING!!!!!
	// https://www.youtube.com/watch?v=46ddlUImiQA

	std::for_each(std::execution::par, m->itv.begin(), m->itv.end(),
		[&](uint32_t y) {
			std::for_each(std::execution::par, m->ith.begin(), m->ith.end(),
			[&](uint32_t x) {
					uint32_t bkc{};
					// bkc
					if (((x / 10) + (y / 10)) % 2 == 0) {
						bkc = 0x161616;
					}
					else {
						bkc = 0x0C0C0C;
					}

					float nscaler = 1.0f / m->mscaler;

					int32_t offX = (((int32_t)m->width - (int32_t)(m->imgwidth * m->mscaler)) / 2);
					int32_t offY = (((int32_t)m->height - (int32_t)(m->imgheight * m->mscaler)) / 2);

					offX += m->iLocX;
					offY += m->iLocY;


					int32_t ptx = (x - offX) * nscaler;
					int32_t pty = (y - offY) * nscaler;


					int margin = 2;
					if (ptx < m->imgwidth && pty < m->imgheight && ptx >= 0 && pty >= 0 && x >= margin && y >= margin && y < m->height - margin && x < m->width - margin) {
						uint32_t c = InvertColorChannels(*GetMemoryLocation(m->imgdata, ptx, pty, m->imgwidth));

						int alpha = (c >> 24) & 255;
						uint32_t doColor = c;
						if (alpha != 255) {
							doColor = lerp(bkc, c, ((float)alpha / 255.0f));
						}
						*GetMemoryLocation(m->scrdata, x, y, m->width) = doColor;

					}
					else {
						if (paintBG) {
							*GetMemoryLocation(m->scrdata, x, y, m->width) = bkc;
						}
					}
				});
		});
	/*
	
	for (uint32_t y = 0; y < m->height; y++) {

		for (uint32_t x = 0; x < m->width; x++) {
	*/
			

		//}
	//}


	if (m->imgwidth > 1) {

		m->CoordLeft = (m->width / 2) - (-m->iLocX) - (int)(((float)(m->imgwidth / 2)) * m->mscaler);
		m->CoordTop = (m->height / 2) - (-m->iLocY) - (int)(((float)(m->imgheight / 2)) * m->mscaler);

		m->CoordRight = (m->width / 2) - (-m->iLocX) + (int)(((float)(m->imgwidth / 2)) * m->mscaler);
		m->CoordBottom = (m->height / 2) - (-m->iLocY) + (int)(((float)(m->imgheight / 2)) * m->mscaler);

	}

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m->hwnd, &p);

	if ((!m->fullscreen && m->height >= 250) || p.y < m->toolheight) {

		// Render the toolbar


		//BLUR FOR TOOLBAR
		if ((GetKeyState('N') & 0x8000) && (GetKeyState('M') & 0x8000)) {
			//gaussianBlur2((uint32_t*)m->scrdata, m->width, m->height, 5, 5, 200, 50, 2.0f);
			boxBlur((uint32_t*)m->scrdata, m->width, m->height, 20);
			//gaussian_blur((uint32_t*)m->scrdata, m->width, m->height, 4.0, m->width);
		}
		else if (m->CoordTop <= m->toolheight) {

			boxBlur((uint32_t*)m->scrdata, m->width, m->toolheight, 20);
			//gaussian_blur((uint32_t*)m->scrdata, m->width, m->toolheight, 4.0, m->width);
		}
		else {

			for (uint32_t y = 0; y < m->toolheight; y++) {
				for (uint32_t x = 0; x < m->width; x++) {
					*GetMemoryLocation(m->scrdata, x, y, m->width) = 0x111111;
				}
			}
		}


		// TOOLBAR CONTAINER
		for (uint32_t y = 0; y < m->toolheight; y++) {
			for (uint32_t x = 0; x < m->width; x++) {
				uint32_t color = 0x050505;
				if (y == m->toolheight - 2) color = 0x303030;
				if (y == m->toolheight - 1) color = 0x000000;

				if (y == 0) color = 0x303030;
				if (y == 1) color = 0x000000;
				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y, m->width);
				*memoryPath = lerp(*memoryPath, color, 0.8f); // transparency
			}
		}

		// drop shadow
		for (uint32_t y = 0; y < 20; y++) {
			for (uint32_t x = 0; x < m->width; x++) {
				uint32_t color = 0x000000;

				uint32_t* memoryPath = GetMemoryLocation(m->scrdata, x, y + m->toolheight, m->width);
				*memoryPath = lerp(*memoryPath, color, (1.0f - ((float)y / 20.0f)) * 0.3f); // transparency
			}
		}

		// BUTTONS

		int p = 5;
		int k = 0;

		if (!m->toolbarData) return;


		for (int i = 0; i < m->maxButtons; i++) {
			if ((p) > (m->width - (m->iconSize))) continue;
			for (uint32_t y = 0; y < m->iconSize; y++) {
				for (uint32_t x = 0; x < m->iconSize; x++) {


					uint32_t l = (*GetMemoryLocation(m->toolbarData, x + k, y, m->widthos));
					int l0 = ((l & 0x00FF0000) >> 16);
					float ll = (float)l0 / 255.0f;
					uint32_t* memoryPath = GetMemoryLocation(m->scrdata, p + x, 6 + y, m->width); \
						if (i != m->selectedbutton) {
							*memoryPath = lerp(*memoryPath, 0xFFFFFF, ll * 0.7f); // transparency
						}
						else {
							*memoryPath = lerp(*memoryPath, 0xFFE0E0, ll * 1.0f); // transparency
						}
				}
			}
			p += m->iconSize + 5; k += m->iconSize + 1;
		}

		if (m->selectedbutton >= 0 && m->selectedbutton < m->maxButtons) {
			// rounded corners: split hover thing into three things

			dDrawFilledRectangle(m->scrdata, m->width, (m->selectedbutton* GetButtonInterval(m) + 2)+1, 4, GetButtonInterval(m)-2, 1, 0xFF8080, 0.2f);
			dDrawFilledRectangle(m->scrdata, m->width, (m->selectedbutton* GetButtonInterval(m) + 2), 5, GetButtonInterval(m), m->toolheight - 10, 0xFF8080, 0.2f);
			dDrawFilledRectangle(m->scrdata, m->width, (m->selectedbutton* GetButtonInterval(m) + 2) + 1, (m->toolheight - 5), GetButtonInterval(m) - 2, 1, 0xFF8080, 0.2f);



			std::string txt = "Error";
			if (m->selectedbutton < m->maxButtons) {
				txt = strtable[m->selectedbutton];
			}

			int loc = 1 + (m->selectedbutton * GetButtonInterval(m) + 2);

			dDrawFilledRectangle(m->scrdata, m->width, loc, m->toolheight + 5, (txt.length() * 8) + 10, 18, 0x000000, 0.8f);
			dDrawRoundedRectangle(m->scrdata, m->width, loc - 1, m->toolheight + 4, (txt.length() * 8) + 12, 20, 0x808080, 0.4f);
			RenderStringFancy(m, txt.c_str(), loc + 4, m->toolheight + 2, 0xFFFFFF, m->scrdata);
		}
	}

	InitFont(m->hwnd, "C:\\Windows\\Fonts\\tahoma.ttf", 10);
	RenderStringFancy(m, "v2.0", m->width-30, 20, 0x808080, m->scrdata);
	InitFont(m->hwnd, "C:\\Windows\\Fonts\\segoeui.ttf", 14);

	if (m->loading) {
		InitFont(m->hwnd, "C:\\Windows\\Fonts\\segoeui.ttf", 20);
		RenderStringFancy(m, "Loading", 12, m->toolheight + 12, 0x000000, m->scrdata);
		RenderStringFancy(m, "Loading", 10, m->toolheight + 10, 0xFFFFFF, m->scrdata);
		InitFont(m->hwnd, "C:\\Windows\\Fonts\\segoeui.ttf", 14);
	}

	// Update window title

	char str[256];
	if (!m->fpath.empty()) {

		sprintf(str, "View Image | %s | %d\%%", m->fpath.c_str(), (int)(m->mscaler * 100.0f));
	}
	else {

		sprintf(str, "View Image | Press F or click a button to begin");
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