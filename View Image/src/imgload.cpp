#include "headers/imgload.hpp"
#include "headers/globalvar.hpp"
#include "stb_image.h"
#include "headers/leftrightlogic.hpp"
#include "stb_image_write.h"
#include "headers/ops.hpp"
#include <shlobj.h>

bool CheckIfStandardFile(const char* filepath) {
	return (isFile(filepath, ".jpeg") || isFile(filepath, ".jpg") || isFile(filepath, ".png") ||
		isFile(filepath, ".tga") || isFile(filepath, ".bmp") || isFile(filepath, ".psd") ||
		isFile(filepath, ".gif") || isFile(filepath, ".hdr") || isFile(filepath, ".pic")
		|| isFile(filepath, ".pmn"));
}


std::string ReplaceBitmapAndMetrics(GlobalParams* m, void*& buffer, const char* standardPath, int* w, int* h) {
	// clean up
	if (buffer) {
		FreeData(buffer);
		buffer = 0;
	}
	*w = 0;
	*h = 0;
	// im really sorry to say that channels varriable, you don't really matter
	// unlike the image width and image height, you just get deallocated automatically
	// by the stack once this function reaches the end of its scope
	// i know you are just an int, but im sorry you are not a necessity for the
	// rest of the program
	int channels;

	if(CheckIfStandardFile(standardPath)){
		buffer = stbi_load(standardPath, w, h, &channels, 4);
		
		if (!buffer) {
			return "Error loading image data";
		}
		InvertAllColorChannels((uint32_t*)buffer, *w, *h);
	}
	else {
		if (isFile(standardPath, ".sfbb")) {
			buffer = decodesfbb(standardPath, w, h);

			if (!buffer) {
				buffer = 0;
				return "Error loading SFBB image data";
			}

		}
		else {
			return "File is in an unsupported format";
		}
	}
	ConvertToPremultipliedAlpha((uint32_t*)buffer, *w, *h);
	
	

	return "Success";
}



#include <filesystem>
std::string FileSaveDialog(GlobalParams* m, HWND hwnd) {
	OPENFILENAME ofn;
	char szFileName[200];

	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	sprintf(szFileName, "IMG_saved %04d-%02d-%02d %02d%02d%02d",
		systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour%24, systemTime.wMinute, systemTime.wSecond);

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	std::filesystem::path p = m->fpath;
	ofn.lpstrInitialDir = p.parent_path().string().c_str();
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = sizeof(szFileName);
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = "png";
	ofn.lpstrTitle = "Save as a PNG File | Warning: This will apply any draft annotations";
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn)) {
		std::string selectedPath = ofn.lpstrFile;
		return selectedPath;
	}
	return "Invalid";
}
/*

void CombineBuffer(GlobalParams* m, uint32_t* first, uint32_t* second, int width, int height, bool invert) {

	m->tempCombineBuffer = (uint32_t*)malloc(width * height * 4);

	for (int i = 0; i < width * height; ++i) {
		// Extract RGBA values for each pixel from the first image
		uint8_t alphaFirst = (first[i] >> 24) & 0xFF;
		uint8_t redFirst = (first[i] >> 16) & 0xFF;
		uint8_t greenFirst = (first[i] >> 8) & 0xFF;
		uint8_t blueFirst = first[i] & 0xFF;

		// Extract RGBA values for each pixel from the second image
		uint8_t alphaSecond = (second[i] >> 24) & 0xFF;
		uint8_t redSecond = (second[i] >> 16) & 0xFF;
		uint8_t greenSecond = (second[i] >> 8) & 0xFF;
		uint8_t blueSecond = second[i] & 0xFF;
		if (invert) {
			redSecond = second[i] & 0xFF;
			greenSecond = (second[i] >> 8) & 0xFF;
			blueSecond = (second[i] >> 16) & 0xFF;

		}

		// Calculate the overlay alpha
		uint8_t overlayAlpha = alphaFirst + alphaSecond * (255 - alphaFirst) / 255;

		// Perform alpha blend
		uint32_t blendedAlpha = alphaFirst + alphaSecond;
		if (blendedAlpha > 255) {
			blendedAlpha = 255;
		}

		float alphaCombine = (float)alphaSecond / 255;
		uint8_t blendedRed = (1 - (alphaCombine)) * redFirst + (alphaCombine)*redSecond;//(redFirst * alphaFirst + redSecond * alphaSecond * (255 - alphaFirst) / (255 * overlayAlpha));
		uint8_t blendedGreen = (1 - (alphaCombine)) * greenFirst + (alphaCombine)*greenSecond;//(greenFirst * alphaFirst + greenSecond * alphaSecond * (255 - alphaFirst) / (255 * overlayAlpha));
		uint8_t blendedBlue = (1 - (alphaCombine)) * blueFirst + (alphaCombine)*blueSecond;//(blueFirst * alphaFirst + blueSecond * alphaSecond * (255 - alphaFirst) / (255 * overlayAlpha));

		// Combine the RGBA values and store in the result buffer
		m->tempCombineBuffer[i] = ((uint8_t)blendedAlpha << 24) | (blendedRed << 16) | (blendedGreen << 8) | blendedBlue;

	}
		///*
		*

		for (int i = 0; i < width * height; i++) {
			uint8_t alpha = (second[i] >> 24) & 0xFF;
			uint8_t invAlpha = 255 - alpha;

			uint8_t backgroundRed = (first[i] >> 16) & 0xFF;
			uint8_t backgroundGreen = (first[i] >> 8) & 0xFF;
			uint8_t backgroundBlue = first[i] & 0xFF;
			uint8_t backgroundA = (first[i] >> 24) & 0xFF;

			uint8_t overlayRed = (second[i] >> 16) & 0xFF;
			uint8_t overlayGreen = (second[i] >> 8) & 0xFF;
			uint8_t overlayBlue = second[i] & 0xFF;
			if (invert) {
				overlayRed = second[i] & 0xFF;
				overlayGreen = (second[i] >> 8) & 0xFF;
				overlayBlue = (second[i] >> 16) & 0xFF;
			}

			uint8_t resultRed = (alpha * overlayRed + invAlpha * backgroundRed) / 255;
			uint8_t resultGreen = (alpha * overlayGreen + invAlpha * backgroundGreen) / 255;
			uint8_t resultBlue = (alpha * overlayBlue + invAlpha * backgroundBlue) / 255;


			*(m->tempCombineBuffer+i) = (resultRed << 16) | (resultGreen << 8) | resultBlue | backgroundA; // Full alpha
		}
		// put end thing here

}


void FreeCombineBuffer(GlobalParams* m) {
	if (m->tempCombineBuffer) {
		FreeData(m->tempCombineBuffer);
	}
}
*/

void PrepareSaveImage(GlobalParams* m) {
	std::string res = FileSaveDialog(m, m->hwnd);
	if (res != "Invalid") {

		m->loading = true;
		RedrawSurface(m);
		//CombineBuffer(m, (uint32_t*)m->imgdata, (uint32_t*)m->imgannotate, m->imgwidth, m->imgheight, true);
		InvertAllColorChannels((uint32_t*)m->imgdata, m->imgwidth, m->imgheight);
		stbi_write_png(res.c_str(), m->imgwidth, m->imgheight, 4, m->imgdata, 0);
		InvertAllColorChannels((uint32_t*)m->imgdata, m->imgwidth, m->imgheight);
		//FreeCombineBuffer(m);
		m->loading = false;
		m->shouldSaveShutdown = false;
		RedrawSurface(m);
		OpenImageFromPath(m, res, false);
	}

}
// the bool is whether or not we should continue or not
bool doIFSave(GlobalParams* m) {
	if (m->shouldSaveShutdown == true) {
		int msgboxID = MessageBox(m->hwnd, "Would you like to save changes to the image", "Are you sure?", MB_YESNOCANCEL);
		if (msgboxID == IDYES) {
			PrepareSaveImage(m);
			return false;
		}
		else if (msgboxID == IDNO) {
			// do it anyway
			return true; // yes, continue
		}
		else {
			return false; // no, don't continue
		}
	}
	return true;
}

bool AllocateBlankImage(GlobalParams* m, uint32_t color) {
	if (!doIFSave(m)) {
		m->loading = false;
		return false;
	}

	// remember when I drew with the small brush and when I clicked blank it cleared the undo. yeah, the order, I switched it
	m->undoData.clear();
	m->undoStep = 0;
	m->drawmode = false;
	TurnOffDraw(m);

	clear_kvector();

	m->loading = true;
	RedrawSurface(m);
	//Sleep(430);

	m->fpath = "Untitled";

	if (m->imgdata) {
		FreeData(m->imgdata);
	}
	if (m->imgoriginaldata) {
		FreeData(m->imgoriginaldata);
	}
	//if (m->imgannotate) {
	//	FreeData(m->imgannotate);
	//}
	// put thing here

	int bimgw = 1280;
	int bimgh = 720;
	m->imgwidth = bimgw;
	m->imgheight = 720;
	m->imgdata = malloc(bimgw * 720 *4);
	m->imgoriginaldata = malloc(bimgw * 720 *4);
	for (int y = 0; y < 720; y++) {
		for (int x = 0; x < bimgw; x++) {
			*GetMemoryLocation(m->imgdata, x, y, bimgw, 720) = color;
			*GetMemoryLocation(m->imgoriginaldata, x, y, bimgw, 720) = color;
		}
	}
	

	if (!m->imgdata || !m->imgoriginaldata) {
		MessageBox(m->hwnd, "Error Loading Image: Big memory error", "Big Error", MB_OK | MB_ICONERROR);
		if (m->imgdata) {
			FreeData(m->imgdata);
		}
		m->imgwidth = 0;
		m->imgheight = 0;
	}

	//m->imgannotate = malloc(m->imgwidth * m->imgheight * 4);

	//memset(m->imgannotate, 0x00, m->imgwidth * m->imgheight * 4);

	// Auto-zoom
	autozoom(m);
	m->shouldSaveShutdown = false;

	m->loading = false;

	RedrawSurface(m);
	return true;
}

// openimagefrompath function here
bool OpenImageFromPath(GlobalParams* m, std::string kpath, bool isLeftRight) {

	m->undoData.clear();
	m->undoStep = 0;
	m->drawmode = false;
	TurnOffDraw(m);

	if (!isLeftRight) {
		clear_kvector();
	}
	
	m->loading = true;
	RedrawSurface(m);
	//Sleep(430);
	
	if (!doIFSave(m)) {
		m->loading = false;
		return false;
	}
	
	m->fpath = kpath;
	
	if (m->imgdata) {
		FreeData(m->imgdata);
	}
	if (m->imgoriginaldata) {
		FreeData(m->imgoriginaldata);
	}
	//if (m->imgannotate) {
	//	FreeData(m->imgannotate);
	//}
	// put thing here
	//auto start = std::chrono::high_resolution_clock::now();

	//
	std::string error = ReplaceBitmapAndMetrics(m, m->imgdata, kpath.c_str(), &m->imgwidth, &m->imgheight);
	if (error != "Success" || !m->imgdata) {
		if (error == "Success") {
			error = "Unknown Error";
		}

		MessageBox(m->hwnd, error.c_str(), "Failed to load", MB_OK | MB_ICONERROR);
		m->fpath = "";
		// should have already cleaned up
		ResetCoordinates(m);
		m->mscaler = 1.0f;
	}
	else {
		// Success
		m->imgoriginaldata = malloc(m->imgwidth * m->imgheight * 4);
		memcpy(m->imgoriginaldata, m->imgdata, m->imgwidth * m->imgheight * 4);
		autozoom(m);
		m->smoothing = ((m->imgwidth * m->imgheight) > 625);
	}
	//
	//m->imgannotate = malloc(m->imgwidth * m->imgheight * 4);

	//memset(m->imgannotate, 0x00, m->imgwidth * m->imgheight * 4);

	m->shouldSaveShutdown = false;

	m->loading = false;

	RedrawSurface(m);
	return true;
}



std::string FileOpenDialog(HWND hwnd) {
	OPENFILENAME ofn = { 0 };
	TCHAR szFile[260] = { 0 };
	// Initialize remaining fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Supported Images (*.sfbb *.jpeg *.jpg *.png *.tga *.bmp *.psd; .gif; .hdr; .pic; .pnm)\0*.sfbb;*.jpeg;*.jpg;*.png;*.tga;*.bmp;*.psd;*.gif;*.hdr;*.pic;*.pnm\0Every File\0*";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		return std::string(ofn.lpstrFile);
	}

	return std::string("Invalid");
}

void PrepareOpenImage(GlobalParams* m) {
	std::string res = "Invalid";

	
	res = FileOpenDialog(m->hwnd);
	


	if (res != "Invalid") {
		//m->imgwidth = 0;
		OpenImageFromPath(m, res, false);
		m->shouldSaveShutdown = false;
	} 
}
