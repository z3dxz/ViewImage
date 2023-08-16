#include "headers/imgload.h"
#include "headers/globalvar.h"
#include "stb_image.h"
#include "headers/leftrightlogic.h"
#include "stb_image_write.h"
#include "headers/ops.h"

bool CheckIfStandardFile(const char* filepath) {
	return (isFile(filepath, ".jpeg") || isFile(filepath, ".jpg") || isFile(filepath, ".png") ||
		isFile(filepath, ".tga") || isFile(filepath, ".bmp") || isFile(filepath, ".psd") ||
		isFile(filepath, ".gif") || isFile(filepath, ".hdr") || isFile(filepath, ".pic")
		|| isFile(filepath, ".pmn"));
}

void* GetStandardBitmap(GlobalParams* m, const char* standardPath, int* w, int* h) {
	void* id;
	// im really sorry to say that channels varriable, you don't really matter
	// unlike the image width and image height, you just get reallocated automatically
	// by the stack once this function reaches the end of its scope
	// i know you are just an int, but im sorry you are not just neccissary for the
	// rest of the program
	int channels;

	if(CheckIfStandardFile(standardPath)){
		id = stbi_load(standardPath, w, h, &channels, 4);

		if (!id) {
			MessageBox(m->hwnd, "Unable to load primary image", "Unknown Error", MB_OK);
			return 0;
		}
	}
	else {
		MessageBox(m->hwnd, "Failed to load because the provided file path is not in standard format", "Failed to load", MB_OK);
		return 0;
	}
	return id;
}

void ShowMessageBox(const std::string& message) {
	MessageBoxA(NULL, message.c_str(), "Folder Name", MB_OK);
}


std::string FileSaveDialog(HWND hwnd) {
	OPENFILENAME ofn;
	TCHAR szFile[260] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = "png";
	ofn.lpstrTitle = "Save as a PNG File";
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn)) {
		std::string selectedPath = ofn.lpstrFile;
		return selectedPath;
	}
	return "Invalid";
}

void PrepareSaveImage(GlobalParams* m) {
	std::string res = FileSaveDialog(m->hwnd);
	if (res != "Invalid") {

		m->loading = true;
		RedrawImageOnBitmap(m);
		stbi_write_png(res.c_str(), m->imgwidth, m->imgheight, 4, m->imgdata, 0);

		m->loading = false;
		RedrawImageOnBitmap(m);
	}

}


bool OpenImageFromPath(GlobalParams* m, std::string kpath, bool isLeftRight) {
	if (!isLeftRight) {
		clear_kvector();
	}
	
	m->loading = true;
	RedrawImageOnBitmap(m);


	if (m->shouldSaveShutdown == true) {
		int msgboxID = MessageBox(m->hwnd, "Would you like to save changes to the image", "Are you sure?", MB_YESNOCANCEL);
		if (msgboxID == IDYES) {
			PrepareSaveImage(m);
		}
		else if(msgboxID == IDNO){
			// do it anyway

		}
		else {
			return false;
		}
	}
	
	m->fpath = kpath;
	
	if (m->imgdata) {
		free(m->imgdata);
	}

	if (CheckIfStandardFile(kpath.c_str())) {
		m->imgdata = GetStandardBitmap(m, kpath.c_str(), &m->imgwidth, &m->imgheight);
	}
	else {

		std::string k0 = m->cd + "\\custom_formats\\";


		WIN32_FIND_DATAA findFileData;
		HANDLE hFind = FindFirstFileA((k0 + "\\*").c_str(), &findFileData);

		if (hFind == INVALID_HANDLE_VALUE) {
			MessageBox(m->hwnd, "Error: No File Handle for Recursive Cycling", "Error", MB_OK);
		}

		do {
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {

					if (isFile(kpath.c_str(), findFileData.cFileName)) {
						std::string applicationPath = k0 + findFileData.cFileName + "\\target.exe";

						DeleteFile("D:\\_f_.png");

						SHELLEXECUTEINFO shExInfo = { 0 };
						shExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // This flag is crucial
						shExInfo.lpVerb = "open";
						shExInfo.lpFile = applicationPath.c_str();
						shExInfo.lpParameters = ("\""+kpath+"\"").c_str();
						shExInfo.nShow = SW_NORMAL;
						if (ShellExecuteEx(&shExInfo)) {
							int k = m->width;
							WaitForSingleObject(shExInfo.hProcess, INFINITE);
							//Sleep(1);
							m->imgdata = GetStandardBitmap(m, "D:\\_f_.png", &m->imgwidth, &m->imgheight);
							// Close the process handle
							CloseHandle(shExInfo.hProcess);
							DeleteFile("D:\\_f_.png");
						}
						else {
							exit(0);
							return 0;
						}
						//HINSTANCE hInstance = ShellExecuteEx(&shExInfo);//(NULL, "open", applicationPath.c_str(), kpath.c_str(), NULL, SW_SHOWNORMAL);
							
					}
				}
			}
		} while (FindNextFileA(hFind, &findFileData) != 0);

		FindClose(hFind);
		
	}

	if (!m->imgdata) {
		MessageBox(m->hwnd, "Error Loading Image", "Error", MB_OK);
		if (m->imgdata) {
			free(m->imgdata);
		}
		m->imgwidth = 0;
		m->imgheight = 0;

	}

	// Auto-zoom
	autozoom(m);

	m->shouldSaveShutdown = false;

	m->loading = false;

	RedrawImageOnBitmap(m);
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
	ofn.lpstrFilter = "Supported Images (*.jpeg *.jpg *.png *.tga *.bmp *.psd; .gif; .hdr; .pic; .pnm; .m45)\0*.m45;*.jpeg;*.jpg;*.png;*.tga;*.bmp;*.psd;*.gif;*.hdr;*.pic;*.pnm\0All Images\0*.*";
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
	std::string res = FileOpenDialog(m->hwnd);
	if (res != "Invalid") {
		m->imgwidth = 0;
		OpenImageFromPath(m, res, false);
	}
	m->shouldSaveShutdown = false;
}